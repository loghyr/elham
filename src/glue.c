/*
 * Copyright (c) 2002-2004, Network Appliance, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *      Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *      Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *      Neither the name of the Network Appliance, Inc. nor the names of
 *      its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
                                                                                                      
/*
 * $Id$
 */

/*
 * Abstraction of the file i/o operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined (unix)
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <errno.h>

#include "elham.h"

/*
 * -----------------------------------------------------------------------
 * File manipulation routines.
 * -----------------------------------------------------------------------
 */

void
syserror (void)
{
#if defined (unix)
	fprintf(stderr, "%s (%d)\n", strerror(errno), errno);
#else
	LPVOID          msgbuf;
	DWORD           err;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL, (err = GetLastError()),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) & msgbuf, 0, NULL);

	*((LPTSTR) msgbuf + (strlen(msgbuf) - 2)) = 0;	/* chop CR+LF */
	fprintf(stderr, "%s (%d)\n", msgbuf, err);

	LocalFree(msgbuf);
#endif
}

void
eh_fatal (char *szFmt, ...)
{
	va_list		vargPtr;

	va_start(vargPtr, szFmt);
	vfprintf(stderr, szFmt, vargPtr);
	va_end(vargPtr);

	exit(1);
}

void
eh_CloseFile (FileStruct *pfs)
{
#if defined (unix)
	int	iRC;
#endif

	eh_ASSERT(pfs != NULL);

	if (pfs->fd == INVALID_HANDLE_VALUE) {
		return;
	}

#if !defined (unix)
	CloseHandle(pfs->fd);
#else

	/*
	 * Now unlock the file if needed.
	 */
	pfs->byteShare = pfs->shareMode;

	/*
	 * We might try something with Solaris's F_SHARE and F_UNSHARE,
	 * but why?
	 */
	switch (pfs->shareMode) {
	case (smRead):
	case (smWrite):
	case (smAll):
		pfs->oStart = 0;
		pfs->oRange = 0;
		pfs->bWait = ROLL_D2(false);
		if (pfs->bFullLocked) {
			eh_ReleaseLock(pfs);
		}
		break;
	case (smNone):
		break;
	default:
		eh_fatal("%s(%d): Close failed on |%s|: lock mode is %d\n",
			      __FILE__, __LINE__, pfs->szFile, pfs->shareMode);
		break;
	}

	iRC = close(pfs->fd);
	if (iRC < 0) {
		fprintf(stderr, "Close failed on |%s|\n", pfs->szFile);
		syserror();
	}
#endif

	pfs->bFullLocked = false;
	pfs->fd = INVALID_HANDLE_VALUE;
	return;
}

Filedesc_t
eh_OpenFile (FileStruct *pfs)
{
#if defined (unix)
	int		oflag = 0;
	bool		b;
#else
	DWORD		dwAccess = 0;
	DWORD		dwShare = 0;
	DWORD		dwCreationDisposition = OPEN_ALWAYS;
	DWORD		dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

	SECURITY_ATTRIBUTES *psec = NULL;
#endif

	eh_ASSERT(pfs != NULL);

#if defined (unix)
	switch (pfs->access) {
	case (amWrite):
		oflag = O_WRONLY;
		break;
	case (amRead):
		oflag = O_RDONLY;
		break;
	case (amRW):
		oflag = O_RDWR;
		break;
	default:
		eh_fatal("%s(%d): Open failed on |%s|: open mode is %d\n",
			__FILE__, __LINE__, pfs->szFile, pfs->access);
		break;
	}

	if (pfs->bCreate) {
		oflag |= O_CREAT;
	}

	if (pfs->bAppend) {
		oflag |= O_APPEND;
	}

#if defined (O_LARGEFILE)
	oflag |= O_LARGEFILE;
#endif

	/*
	 * Should never need this, right?
	 */
	oflag |= O_BINARY;

	pfs->fd = open(pfs->szFile, oflag, FILE_MODE);
	if (pfs->fd < 0) {
		pfs->fd = INVALID_HANDLE_VALUE;
		fprintf(stderr, "Open failed on |%s|\n", pfs->szFile);
		syserror();
		return(pfs->fd);
	}

	/*
	 * Now lock the file if needed, note we may not implement
	 * true deny semantics as defined by the Windows case, but we
	 * are denying access to the file.
	 */
	pfs->byteShare = pfs->shareMode;

	/*
	 * We might try something with Solaris's F_SHARE and F_UNSHARE,
	 * but why?  They are not very portable, are they?
	 */
	switch (pfs->shareMode) {
	case (smRead):
	case (smWrite):
	case (smAll):
		pfs->oStart = 0;
		pfs->oRange = 0;
		pfs->bWait = ROLL_D2(false);
		b = eh_CreateLock(pfs);
		break;
	case (smNone):
		b = true;
		break;
	default:
		eh_fatal("%s(%d): Open failed on |%s|: lock mode is %d\n",
			__FILE__, __LINE__, pfs->szFile, pfs->shareMode);
		break;
	}

	/*
	 * Could not lock the file.
	 */
	if (!b) {
		eh_CloseFile(pfs);
	}
#else
	/*
	 * Note: This is how Windows does file level locking.
	 * See: http://www.netapp.com/tech_library/3024.html.
	 */
	switch (pfs->shareMode) {
	case (smAll):
		dwShare = 0;
		break;
	case (smRead):
		dwShare = FILE_SHARE_WRITE;
		break;
	case (smWrite):
		dwShare = FILE_SHARE_READ;
		break;
	case (smNone):
		dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;
	default:
		eh_fatal("%s(%d): Open failed on |%s|: lock mode is %d\n",
			__FILE__, __LINE__, pfs->szFile, pfs->shareMode);
		break;
	}

	switch (pfs->access) {
	case (amWrite):
		dwAccess = GENERIC_WRITE;
		break;
	case (amRead):
		dwAccess = GENERIC_READ;
		break;
	case (amRW):
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		break;
	default:
		eh_fatal("%s(%d): Open failed on |%s|: access mode is %d\n",
			__FILE__, __LINE__, pfs->szFile, pfs->access);
		break;
	}

	pfs->fd = CreateFile(pfs->szFile, dwAccess, dwShare, psec,
			 dwCreationDisposition, dwFlagsAndAttributes, NULL);
	if (pfs->fd == INVALID_HANDLE_VALUE) {
		printf("Open failed on |%s|\n", pfs->szFile);
		syserror();
	}
#endif

	switch (pfs->shareMode) {
	case (smRead):
	case (smWrite):
	case (smAll):
		pfs->bFullLocked = true;
		break;
	case (smNone):
		pfs->bFullLocked = false;
		break;
	default:
		eh_fatal("%s(%d): Switch out of range\n",
				__FILE__, __LINE__, pfs->shareMode);
		break;
	}
	
	return(pfs->fd);
}

/*
 * -----------------------------------------------------------------------
 * Lock manipulation routines.
 * -----------------------------------------------------------------------
 */

bool
eh_CreateLock (FileStruct *pfs)
{
	int		i;

#if defined (unix)
	int		iRC;
#else
	bool		b;
	DWORD		dwFlags = 0;
#endif

	eh_ASSERT(pfs != NULL);

	if (pfs->fd == INVALID_HANDLE_VALUE) {
		return(false);
	}

	for (i = 0; ; i++) {
#if defined (unix)
		/*
		 * iRC is modified inside the macros.
		 */
		if (pfs->byteShare == smRead) {
			READ_LOCK_IF_ABLE(pfs->fd, pfs->oStart,
					SEEK_SET, pfs->oRange, iRC);
		} else {
			WRITE_LOCK_IF_ABLE(pfs->fd, pfs->oStart,
					SEEK_SET, pfs->oRange, iRC);
		}
	
		if (iRC != -1) {
			return(true);
		}
#else
		pfs->offset.Internal = pfs->offset.InternalHigh =
			pfs->offset.OffsetHigh = 0;
		pfs->offset.hEvent = NULL;
		pfs->offset.Offset = (DWORD)pfs->oStart;
		pfs->offset.OffsetHigh = (DWORD)(pfs->oStart >> 32);

		/* Default of LockFileEx() is to block */
		dwFlags |= LOCKFILE_FAIL_IMMEDIATELY;

		/* Default of LockFileEx() is a shared lock */
		if (pfs->byteShare != smRead) {
			dwFlags |= LOCKFILE_EXCLUSIVE_LOCK;
		}

		b = LockFileEx(pfs->fd, dwFlags, 0, (DWORD)pfs->oRange,
				(DWORD)(pfs->oRange >> 32), &pfs->offset);

		if (b) {
			return(true);
		}
#endif

		/*
		 * Do we want to block locally until we either:
		 * 1) get the resource
		 * 2) give up in case of a dead lock.
		 */
		if (pfs->bWait) {
			/*
			 * Force at least one wait.
			 */
			if (!i) {
				sleep(1);
			} else {
				if (ROLL_D100(false) > 90) {
					return(false);
				} else {
					sleep(1);
				}
			}
		} else {
			return(false);
		}
	}

	return(true);
}

void
eh_ReleaseLock (FileStruct *pfs)
{
#if defined (unix)
#else
	bool		b;
	DWORD		dwFlags = 0;
#endif

	eh_ASSERT(pfs != NULL);

	if (pfs->fd == INVALID_HANDLE_VALUE) {
		return;
	}

#if defined (unix)
	UN_LOCK(pfs->fd, pfs->oStart, SEEK_SET, pfs->oRange);
#else
	pfs->offset.Internal = pfs->offset.InternalHigh =
		pfs->offset.OffsetHigh = 0;
	pfs->offset.hEvent = NULL;
	pfs->offset.Offset = (DWORD)pfs->oStart;
	pfs->offset.OffsetHigh = (DWORD)(pfs->oStart >> 32);

	/* Default of LockFileEx() is to block */
	dwFlags |= LOCKFILE_FAIL_IMMEDIATELY;

	/* Default of LockFileEx() is a shared lock */
	if (pfs->byteShare == smRead) {
		dwFlags |= LOCKFILE_EXCLUSIVE_LOCK;
	}
/*
 * XXX Print status messages.  What about syserror()?
 */
	b = UnlockFileEx(pfs->fd, 0, (DWORD)pfs->oRange,
			(DWORD)(pfs->oRange >> 32), &pfs->offset);
	if (!b) {
		syserror();
	}
#endif

	return;
}

bool
eh_RandomlyByteLockRange (FileStruct *pfs, bool bCreate)
{
	Offset_t	*order = (Offset_t *)NULL;
	Offset_t	i;
	Offset_t	len;
	Offset_t	end;
	Offset_t	i1;
	Offset_t	i2;
	Offset_t	t;

	eh_ASSERT(pfs);

	if (pfs->fd == INVALID_HANDLE_VALUE) {
		fprintf(stderr,
			"CLOSED FILE - %s - trying to access a lock "
			"at offset "LLXFMT"\n",
			pfs->szFile, pfs->oBaseStart);
	}

	if (!bCreate) {
		if (!pfs->bLocked) {
			return(false);
		}
	}

	len = pfs->oBaseLen;
	end = 2 * len;

	order = (Offset_t *)calloc((int)len, sizeof(Offset_t));
	if (!order) {
		eh_fatal("%s(%d): Could not allocate "LLFMT" bytes\n",
			__FILE__, __LINE__, len * sizeof(Offset_t));
	}

	for (i = 0; i < len; i++) {
		order[i] = i;
	}

	/*
	 * Shuffle the cards.
	 */
	for (i = 0; i < end; i++) {
		i1 = ROLL_UINT8(len, false);
		i2 = ROLL_UINT8(len, false);
		t = order[i1];
		order[i1] = order[i2];
		order[i2] = t;
	}

	/*
	 * Now start dealing the locks in order.
	 */
	for (i = 0; i < len; i++) {
		pfs->oStart = pfs->oBaseStart + order[i];
		pfs->oRange = 1;

		if (bCreate) {
			pfs->bLocked = eh_CreateLock(pfs);
			if (!pfs->bLocked) {
				Offset_t j;
				for (j = 0; j < i; j++) {
					pfs->oStart = pfs->oBaseStart +
							order[i];
					pfs->oRange = 1;
					eh_ReleaseLock(pfs);
				}
			}
		} else {
			eh_ReleaseLock(pfs);
		}
	}

	free(order);
	return(pfs->bLocked);
}

void
verifyBase (char *szBase)
{
	if (!isDir(szBase)) {
		eh_fatal("%s(%d): Not a Directory: %s\n",
			__FILE__, __LINE__, szBase);
	}
}

bool
eh_LockFileStruct (FileStruct *pfs, bool bCreate)
{
	eh_ASSERT(pfs);

	if (pfs->bByteLocks) {
		if (pfs->bFullRange) {
			pfs->oStart = pfs->oBaseStart;
			pfs->oRange = pfs->oBaseLen;

			if (bCreate) {
				pfs->bLocked = eh_CreateLock(pfs);
			} else {
				eh_ReleaseLock(pfs);
				pfs->bLocked = false;
			}
		} else {
			eh_RandomlyByteLockRange(pfs, bCreate);
		}
	} else {
#if defined (unix)
		pfs->oStart = 0;
		pfs->oRange = 0;

		if (bCreate) {
			pfs->bLocked = eh_CreateLock(pfs);
		} else {
			eh_ReleaseLock(pfs);
			pfs->bLocked = false;
		}
#else
		/*
		 * Do nothing, we took care of this during the
		 * file open and closes.
		 */
		if (bCreate) {
			pfs->bLocked = true;
		} else {
			pfs->bLocked = false;
		}
#endif
	}

	return(pfs->bLocked);
}

uint8
eh_Read (Filedesc_t fd, void *buf, uint8 bytes)
{
	uint8	c;
	uint4	iRetries = 10;

	eh_ASSERT(buf);

#if defined (unix)
	do {
		c = read(fd, buf, bytes);
		if (c == -1) {
			switch (errno) {
			case (EAGAIN) :
			case (EINTR) :
			case (ENOLINK) :
				/*
				 * Note: We should really reset the
				 * file position.
				 */
				if (iRetries-- > 0) {
					sleep(1);
					break;
				}
			case (EBADF) :
			case (EFAULT) :
			case (EOVERFLOW) :
			default :
				return(c);
				break;
			}
		}
	} while (c == -1);
#else
#endif

	return(c);
}

/*
 * -1 is error.
 * -2 is ENOSPC.
 * -3 is EDQUOT
 *
 * We need to know EDQUOT and ENOSPC in order to adapt
 * to these conditions.
 */
uint8
eh_Write (Filedesc_t fd, void *buf, uint8 bytes)
{
	uint8	c;
	uint4	iRetries = 10;

	bool	b = true;

	eh_ASSERT(buf);

#if defined (unix)
	do {
		c = write(fd, buf, bytes);
		if (c == -1) {
			switch (errno) {
			case (EDQUOT) :

				fprintf(stderr, "quotas exhausted\n");
				return(-3);
				break;

			case (ENOSPC) :

				fprintf(stderr, "space exhausted\n");
				return(-2);
				break;

			case (EAGAIN) :
			case (EINTR) :
			case (ENOLINK) :

				if (b) {
					switch (errno) {
					case (EAGAIN) :
						fprintf(stderr, "EAGAIN");
						break;
					case (EINTR) :
						fprintf(stderr, "EINTR");
						break;
					case (ENOLINK) :
						fprintf(stderr, "ENOLINK");
						break;
					case (ESTALE) :
						fprintf(stderr, "ESTALE");
						break;
					}

					fprintf(stderr, " found on write, trying again\n");

					b = false;
				}

				/*
				 * We should be kind and rewind?
				 */
				if (iRetries-- > 0) {
					sleep(1);
					break;
				}

			case (EBADF) :
			case (EFAULT) :
			case (EFBIG) :
			case (EOVERFLOW) :
			default :
				return(c);
				break;
			}
		}
	} while (c == -1);
#else
#endif

	return(c);
}

int
eh_Rmdir (char *szPath)
{
	uint4	disconnects = 0;

	eh_ASSERT(szPath);
again:

	if (rmdir(szPath) != -1) {
		return(0);
	}

	if (eh_SessionDisconnected()) {
		disconnects++;
		goto again;
	}

	if (disconnects > 0 && errno == ENOENT) {
		return(0);
	}

	return(-1);
}

int
eh_SessionDisconnected (void)
{
#if !defined (unix)
	DWORD e = GetLastError();

	/* these errors occur when the CIFS session dies */
	return (e == ERROR_NETNAME_DELETED     ||
		e == ERROR_UNEXP_NET_ERR       ||
		e == ERROR_DEV_NOT_EXIST       ||
		e == ERROR_REM_NOT_LIST        ||
		e == ERROR_BAD_NETPATH         ||
		e == ERROR_BAD_NET_NAME        ||
		e == ERROR_VC_DISCONNECTED     ||
		e == ERROR_SEM_TIMEOUT         ||
		e == ERROR_NO_LOGON_SERVERS    ||
		e == ERROR_INVALID_HANDLE      ||
		e == ERROR_NOT_SUPPORTED       ||
		e == ERROR_OPERATION_ABORTED);
#endif

	return(false);
}


#if defined (unix)
int
lock_reg (int fd, int cmd, int type, Offset_t offset, int whence, off_t len)
{
#if defined SOLARIS_LARGE_LOCKS
	struct flock64	lock;
#else
	struct flock	lock;
#endif

	int	iRC = 0;

	lock.l_type = type;	/* F_RDLCK, F_WRLCK, F_UNLCK */
	lock.l_start = offset;	/* byte offset, relative to l_whence */
	lock.l_whence = whence;	/* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = len;	/* #bytes (0 means to EOF) */

	iRC = fcntl(fd, cmd, &lock);
	if (iRC == -1) {
		fprintf(stderr, "Could not %s a lock\n",
			type == F_UNLCK ? "unlock" :
				type == F_WRLCK ? "write lock" : "read lock");

		syserror();
	}
	return (iRC);		/* -1 upon error */
}

Pid_t
lock_test (int fd, int type, Offset_t offset, int whence, off_t len)
{
#if defined SOLARIS_LARGE_LOCKS
	struct flock64	lock;
#else
	struct flock	lock;
#endif

	lock.l_type = type;	/* F_RDLCK or F_WRLCK */
	lock.l_start = offset;	/* byte offset, relative to l_whence */
	lock.l_whence = whence;	/* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = len;	/* #bytes (0 means to EOF) */

	if (fcntl(fd, eh_F_GETLK, &lock) == -1) {
		fprintf(stderr, "Could not get a %s lock\n",
			type == F_RDLCK ? "read" : "write");

		return (-1);	/* unexpected error */
	}
	if (lock.l_type == F_UNLCK) {
		return (0);	/* false, region not locked by another proc */
	}
	return (lock.l_pid);	/* true, return positive PID of lock owner */
}
#endif
