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
 * Access to the metafile entries.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#include <errno.h>

#include "elham.h"

/*
 * Actions:
 */
static bool actionCreate (ControlStruct *pcs);
static bool actionRead (ControlStruct *pcs);
static bool actionUnlink (ControlStruct *pcs);
static bool actionModify (ControlStruct *pcs);

char *
actionStringOf (ActionEnum ae)
{
	switch (ae) {
	case (aeCreate) :
		return("CREATE");
	case (aeRead) :
		return("READ");
	case (aeWrite) :
		return("WRITE");
	case (aeDelete) :
		return("DELETE");
	case (aeUnlink) :
		return("UNLINK");
	default :
		eh_fatal("%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, ae);
		break;
	}

	return(NULL);
}

void
verifyActionWeightsOf (ActionWeightsStruct actions, char *szFile, int iLine)
{
	int	i;

	i = actions.read + actions.create + actions.write +
		actions.delete + actions.unlink;

	if (i != 100) {
		eh_fatal("%s(%d): The sum of the action weights %d != 100\n"
			"read = %d\n"
			"create = %d\n"
			"write = %d\n"
			"delete = %d\n"
			"unlink = %d\n",
			szFile, iLine,
			i, actions.read, actions.create, actions.write,
			actions.delete, actions.unlink);
	}
}

char *
useStringOf (UseEnum us)
{
	switch (us) {
	case (usVirgin) :
		return("virgin");
	case (usActive) :
		return("active");
	case (usSoiled) :
		return("soiled");
	default :
		eh_fatal("%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, us);
		break;
	}

	return(NULL);
}

bool
verifyTheRecord (ControlStruct *pcs)
{
	uint4		k;
	uint8		u;
	uint8		u1;
	uint8		rw;
	char		*pc = (char *)NULL;
	char		*pb = (char *)NULL;

	HistoryQueueStruct	*phq = (HistoryQueueStruct *)NULL;

	bool		b = false;
	bool		bBlockCorrupt = false;

	Offset_t	oRet;

	eh_ASSERT(pcs);
	eh_ASSERT(pcs->meta.fd != INVALID_HANDLE_VALUE);
	eh_ASSERT(pcs->data.fd != INVALID_HANDLE_VALUE);

	oRet = lseek(pcs->meta.fd, sizeof(DataFileControlBlock), SEEK_SET);
	if (oRet == -1) {
		fprintf(stderr, "Hit unexpected error in lseek on %s\n", pcs->meta.szFile);
		syserror();

		return(false);
	}

	oRet = lseek(pcs->meta.fd, pcs->hitThisBlock * sizeof(BlockStruct), SEEK_CUR);
	if (oRet == -1) {
		fprintf(stderr, "Hit unexpected error in lseek on %s\n", pcs->meta.szFile);
		syserror();

		return(false);
	}

	rw = readBlock(pcs->meta.fd, &pcs->bs);
	if (rw != sizeof(BlockStruct)) {
		return(false);
	}

	if (pcs->bs.inUse != usActive && !pcs->rs.dumpInvalid) {
		fprintf(stderr, "There is junk in block "
			LLUFMT" - not processing\n", pcs->hitThisBlock);

		return(false);
	}

	/*
	 * We will read the block before we process it, so both areas
	 * are valid storage.
	 */
	pb = pcs->rs.cooked;

	init_genrand(pcs->bs.seed, true);
	advance_flips(pcs->bs.flips, true);

	pcs->rs.pos = pcs->bs.offset;
	rw = readRecord(pcs->data.fd, &pcs->rs);
	if (rw != pcs->rs.blockSize) {
		return(false);
	}

	phq = windHistory(pcs, __FILE__, __LINE__);
	if (!phq) {
	}

	gettimeofday(&phq->hs.time, NULL);
	phq->hs.seed = pcs->bs.seed;
	phq->hs.action = aeRead;
	phq->hs.corrupt = pcs->bs.corrupt;
	phq->hs.inUse = pcs->bs.inUse;
	phq->hs.block = pcs->hitThisBlock;
	phq->hs.flips = pcs->bs.flips;
	phq->hs.pattern = pcs->bs.pattern;
	phq->hs.blockSize =  pcs->bs.blockSize;
	phq->hs.pid = pcs->pid;
	phq->hs.mid = 0xdeade4e5;

	oRet = lseek(pcs->meta.fd, sizeof(DataFileControlBlock), SEEK_SET);
	if (oRet == -1) {
		fprintf(stderr, "Hit unexpected error in lseek on %s\n",
			pcs->meta.szFile);
		syserror();

		return(false);
	}

	oRet = lseek(pcs->meta.fd, pcs->hitThisBlock * sizeof(BlockStruct),
			SEEK_CUR);
	if (oRet == -1) {
		fprintf(stderr, "Hit unexpected error in lseek on %s\n",
			pcs->meta.szFile);
		syserror();

		return(false);
	}

	pcs->bs.accessPid = pcs->bs.readPid = pcs->pid;
	pcs->bs.accessTime = pcs->bs.readTime = phq->hs.time;

	rw = writeBlock(pcs->meta.fd, &pcs->bs);
	if (rw != sizeof(BlockStruct)) {
	}

	if (pcs->rs.print) {
		printRecord(pcs->rs, pcs->hitThisBlock);
	}

	if (pcs->bs.inUse != usActive && !pcs->rs.verifyJunk) {
		fprintf(stderr, "There is junk in block "
			LLUFMT" - not verifying\n", pcs->hitThisBlock);

		return(false);
	}

	for (k = 0; k < pcs->rs.blockSize; k++, pc++) {
		if (!(k % sizeof(uint8))) {
			switch (pcs->bs.inUse) {
			case (usVirgin) :
				memset(&u1, '\0', sizeof(uint8));
				break;
			case (usActive) :
				u1 = genrand_int64(true);
				break;
			case (usSoiled) :
				u1 = SOILED_BLOCK_PATTERN;
				break;
			default :
				eh_fatal("%s(%d): Switch out of range - %d\n",
						__FILE__, __LINE__,
						pcs->bs.inUse);
				break;
			}


			/*
			 * Even if we have less than 8 bytes to compare,
			 * we will still end up chopping off the same ones.
			 */
			byteSwapData((char *)&u, (char *)&u1, sizeof(uint8));

			pc = (char *)&u;
		}

		if (pb[k] != *pc) {
			fprintf(stderr, "CORRUPTION: %s byte %#02x at offset "
				LL8XFMT" does not match expected "
				"pattern %#02x (byte %d of "LLXFMT" "
				")\n", pcs->data.szFile,
				pb[k] & 0XFF, pcs->rs.pos + k,
				*pc & 0xFF, pc - (char *)&u, u);

			bBlockCorrupt = pcs->bs.corrupt;
			pcs->bs.corrupt = true;

			printBlock(pcs->bs, pcs->hitThisBlock,
					pcs->dfcb.blocks);
			b = true;
			break;
		}
	}

	/*
	 * Need to update the history at this time.
	 * Record corruption if found.
	 * Also update the DFCB such that no other
	 * instance can modify the file.
	 */
	if (b) {
		phq = windHistory(pcs, __FILE__, __LINE__);
		if (!phq) {
		}

		gettimeofday(&phq->hs.time, NULL);
		phq->hs.seed = pcs->bs.seed;
		phq->hs.action = aeRead;
		phq->hs.corrupt = true;
		phq->hs.block = pcs->hitThisBlock;
		phq->hs.flips = pcs->bs.flips;
		phq->hs.pattern = pcs->bs.pattern;
		phq->hs.blockSize =  pcs->bs.blockSize;
		phq->hs.pid = pcs->pid;
		phq->hs.mid = 0xdeade4e5;

		oRet = lseek(pcs->meta.fd, sizeof(DataFileControlBlock),
				SEEK_SET);
		if (oRet == -1) {
			fprintf(stderr,
				"Hit unexpected error in lseek on %s\n",
				pcs->meta.szFile);
			syserror();

			return(false);
		}

		/*
		 * Block modifications to this file.
		 */
		if (!pcs->dfcb.corrupt) {
			pcs->dfcb.corrupt = true;
			rw = writeDFCB(pcs->meta.fd, &pcs->dfcb);
			if (rw != sizeof(DataFileControlBlock)) {
			}
		}

		if (!bBlockCorrupt) {
			oRet = lseek(pcs->meta.fd,
				pcs->hitThisBlock * sizeof(BlockStruct),
				SEEK_CUR);
			if (oRet == -1) {
				fprintf(stderr,
					"Hit unexpected error in lseek on %s\n",
					pcs->meta.szFile);
				syserror();

				return(false);
			}

			rw = writeBlock(pcs->meta.fd, &pcs->bs);
			if (rw != sizeof(DataFileControlBlock)) {
			}
		}

		if (pcs->meta.fd != INVALID_HANDLE_VALUE) {
			eh_CloseFile(&pcs->meta);
		}

		if (pcs->data.fd != INVALID_HANDLE_VALUE) {
			eh_CloseFile(&pcs->data);
		}

		exit(1);
	}

	return(b);
}

bool
generateTheRecord (ControlStruct *pcs)
{
	uint8		k;
	uint8		*pu = (uint8 *)NULL;
	char		*pc = (char *)NULL;

	uint8		rw;

	HistoryQueueStruct	*phq = (HistoryQueueStruct *)NULL;

	Offset_t	oRet;

	eh_ASSERT(pcs);
	eh_ASSERT(pcs->meta.fd != INVALID_HANDLE_VALUE);
	eh_ASSERT(pcs->data.fd != INVALID_HANDLE_VALUE);

	if (pcs->dfcb.corrupt || pcs->bs.corrupt) {
		printf("Block "LLUFMT" is corrupt, not "
			"overwriting it.\n", pcs->hitThisBlock);
	}

	init_genrand(pcs->bs.seed, true);
	advance_flips(pcs->bs.flips, true);

	phq = windHistory(pcs, __FILE__, __LINE__);
	if (!phq) {
	}

	gettimeofday(&phq->hs.time, NULL);
	if (pcs->takeAction == aeDelete) {
		phq->hs.pattern = pcs->bs.pattern = SOILED_BLOCK_PATTERN;
		pcs->bs.inUse = usSoiled;
	} else {
		phq->hs.pattern = pcs->bs.pattern = genrand_int64(true);
		pcs->bs.inUse = usActive;
	}

	pcs->rs.pos = pcs->bs.offset;

	/*
	 * We know we have 8 extra bytes, so lets
	 * copy over the entire 8 byte block and
	 * let the transfer code handle it.
	 */
	if (pcs->rs.blockSize < sizeof(uint8)) {
		pc = (char *)&pcs->bs.pattern;
		for (k = 0; k < 8; k++) {
			pcs->rs.block[k] = pc[k];
		}
	} else {
		/*
		 * XXX: Code review this chunk.
		 */
		pu = (uint8 *)pcs->rs.block;
		*pu++ = pcs->bs.pattern;
		for (k = sizeof(uint8);
			k < pcs->rs.blockSize - sizeof(uint8);
			k += sizeof(uint8)) {
			if (pcs->takeAction == aeDelete) {
				*pu++ = pcs->bs.pattern;
			} else {
				*pu++ = genrand_int64(true);
			}
		}

		/*
		 * Grab the last one, which amazingly enough,
		 * we have already provided space for!
		 */
		if (pcs->takeAction == aeDelete) {
			*pu++ = pcs->bs.pattern;
		} else {
			*pu++ = genrand_int64(true);
		}
	}

	oRet = lseek(pcs->meta.fd, sizeof(DataFileControlBlock), SEEK_SET);
	oRet = lseek(pcs->meta.fd, pcs->hitThisBlock * sizeof(BlockStruct),
		SEEK_CUR);
	pcs->bs.accessPid = pcs->bs.readPid = pcs->pid;
	pcs->bs.accessTime = pcs->bs.readTime = phq->hs.time;

	pcs->bs.flips = phq->hs.flips;

	rw = writeBlock(pcs->meta.fd, &pcs->bs);
	if (rw != sizeof(BlockStruct)) {
	}

	rw = writeRecord(pcs->data.fd, &pcs->rs);
	if (rw != pcs->rs.blockSize) {
	}

	phq->hs.seed = pcs->bs.seed;
	phq->hs.action = pcs->takeAction;
	phq->hs.corrupt = false;
	phq->hs.inUse = pcs->bs.inUse;
	phq->hs.block = pcs->hitThisBlock;
	phq->hs.flips = pcs->bs.flips;
	phq->hs.blockSize =  pcs->bs.blockSize;
	phq->hs.block = pcs->hitThisBlock;
	phq->hs.pid = pcs->pid;
	phq->hs.mid = 0xdeade4e5;

	return(true);
}

HistoryQueueStruct *
windHistory (ControlStruct *pcs, char *szLine, int iLine)
{
	HistoryQueueStruct	*p;

	eh_ASSERT(pcs);
	eh_ASSERT(szLine);

	p = (HistoryQueueStruct *)calloc(1, sizeof(HistoryQueueStruct));
	if (!p) {
		eh_fatal("%s(%d): Could not allocate %d bytes\n",
			szLine, iLine, sizeof(HistoryQueueStruct));
	}

	if (pcs->hqTail) {
		pcs->hqTail->next = p;
		p->prev = pcs->hqTail;
	} else {
		pcs->hq.next = p;
		p->prev = &pcs->hq;
	}

	pcs->hqTail = p;

	return(p);
}

void
unwindHistory (ControlStruct *pcs)
{
	HistoryQueueStruct	*p;
	HistoryQueueStruct	*prev;

	uint8			rw;

	eh_ASSERT(pcs);

	/*
	 * Nothing generated.
	 */
	if (!pcs->hq.next) {
		return;
	}

	eh_ASSERT(pcs->history.szFile[0] != '\0');
	eh_ASSERT(pcs->hqTail);

	/*
	 * Really want to wait until it does get opened.
	 */
	pcs->history.iOpenFailures = 0;
	while (1) {
		pcs->history.bFullRange = true;
		pcs->history.bByteLocks = false;
		pcs->history.bWait = true;
		pcs->history.bCreate = true;
		pcs->history.bAppend = true;
		pcs->history.access = amRW;
		pcs->history.shareMode = smAll;

		if (eh_OpenFile(&pcs->history) != INVALID_HANDLE_VALUE) {
			pcs->history.iOpenFailures = 0;
			break;
		}

		if (!(pcs->history.iOpenFailures++ % 60)) {
			eh_fatal("%s(%d): Could not open %s\n",
				__FILE__, __LINE__, pcs->history.szFile);
			sleep(1);
		}

		sleep(1);
	}

	for (p = pcs->hqTail; p != NULL && p != &pcs->hq; p = prev) {
		prev = p->prev;

		rw = writeHistory(pcs->history.fd, &p->hs);
		if (rw != sizeof(HistoryStruct)) {
		}

		free(p);
	}

	eh_ASSERT(p);
	pcs->hqTail = (HistoryQueueStruct *)NULL;
	pcs->hq.next = (HistoryQueueStruct *)NULL;

	if (pcs->history.fd != INVALID_HANDLE_VALUE) {
		eh_CloseFile(&pcs->history);
	}

	pcs->history.szFile[0] = '\0';
}

/*
 * No input error checking on the action* routines as
 * that will be done by the wrapper.
 */

bool
decideOnAction (ControlStruct *pcs)
{
	ActionWeightsStruct	actions;

	uint4		change;

	bool		bRC = true;

	eh_ASSERT(pcs);
	eh_ASSERT(!pcs->rs.block);
	eh_ASSERT(!pcs->rs.cooked);
	eh_ASSERT(!pcs->shuffled);
	eh_ASSERT(!pcs->hq.next);
	eh_ASSERT(!pcs->hqTail);
	eh_ASSERT(pcs->data.fd == INVALID_HANDLE_VALUE);
	eh_ASSERT(pcs->meta.fd == INVALID_HANDLE_VALUE);
	eh_ASSERT(pcs->history.fd == INVALID_HANDLE_VALUE);

	/*
	 * Determine the action we are going to take under our
	 * state conditions.
	 */
	change = ROLL_D100(false);
	actions.read = pcs->actions.read;
	actions.create = pcs->actions.create + actions.read;
	actions.write = pcs->actions.write + actions.create;
	actions.delete = pcs->actions.delete + actions.write;
	actions.unlink = pcs->actions.unlink + actions.delete;

	/*
	 * Each function is responsible for filling in the name
	 * of the history file and for putting items on the queue
	 * of history actions for this current action.
	 */
	if (change < actions.read) {
		pcs->takeAction = aeRead;
		bRC = actionRead(pcs);
	} else if (change < actions.create) {
		pcs->takeAction = aeCreate;
		bRC = actionCreate(pcs);
	} else if (change < actions.write) {
		pcs->takeAction = aeWrite;
		bRC = actionModify(pcs);
	} else if (change < actions.delete) {
		pcs->takeAction = aeDelete;
		bRC = actionModify(pcs);
	} else if (change < actions.unlink) {
		pcs->takeAction = aeUnlink;
		bRC = actionUnlink(pcs);
	} else {
		fprintf(stderr, DOCFMT" is %ld\n", "-wtread", actions.read);
		fprintf(stderr, DOCFMT" is %ld\n", "-wtcreate", actions.create);
		fprintf(stderr, DOCFMT" is %ld\n", "-wtwrite", actions.write);
		fprintf(stderr, DOCFMT" is %ld\n", "-wtdelete", actions.delete);
		fprintf(stderr, DOCFMT" is %ld\n", "-wtunlink", actions.unlink);
		eh_fatal("%s(%d): Could not determine an action"
			" our random number generator picked one"
			" greater than allowed (%d > 100).\n",
			__FILE__, __LINE__, change);
	}

	pcs->hitThisBlock = -1;

	if (pcs->shuffled) {
		free(pcs->shuffled);
		pcs->shuffled = (uint8 *)NULL;
	}

	if (pcs->rs.cooked) {
		free(pcs->rs.cooked);
		pcs->rs.cooked = (char *)NULL;
	}

	if (pcs->rs.block) {
		free(pcs->rs.block);
		pcs->rs.block = (char *)NULL;
	}

	if (pcs->meta.fd != INVALID_HANDLE_VALUE) {
		eh_CloseFile(&pcs->meta);
	}

	if (pcs->data.fd != INVALID_HANDLE_VALUE) {
		eh_CloseFile(&pcs->data);
	}

	/*
	 * Now record the actions we took inside the history file
	 * for this data file.
	 */
	unwindHistory(pcs);

	if (!bRC) {
		sleep(1);
	}

	return(bRC);
}

static bool
actionCreate (ControlStruct *pcs)
{
	uint8		j;
	uint8		jBound;
	uint8		uToFind;
	uint8		uDepth;
	uint8		iPick;

	uint8		rw;

	bool		b = true;
	bool		bFound = true;

	PathStackStruct	*stack = (PathStackStruct *)NULL;
	PathStackStruct	*p = (PathStackStruct *)NULL;

	HistoryQueueStruct	*phq = (HistoryQueueStruct *)NULL;

	eh_ASSERT(pcs);

	/*
	 * Give it the old college try.
	 */
	bFound = false;
	jBound = MAX(pcs->state.files.min.hard, 60);
	for (j = 0; j < jBound; j++) {
		uToFind = ROLL_UINT8(pcs->state.files.max.limit, false);

		uDepth = 0;

		stack = buildPathStackOf(pcs, NULL, fdFile, uToFind, &uDepth);
		strncpy(pcs->data.szFile, pcs->data.szBase, EH_PATH_MAX);
		strncpy(pcs->meta.szFile, pcs->meta.szBase, EH_PATH_MAX);
		strncpy(pcs->history.szFile, pcs->history.szBase, EH_PATH_MAX);

		for (p = stack; p != NULL; p = p->next) {
			/*
			 * XXX: Need to guard!
			 */
			strcat(pcs->data.szFile, p->szName);
			strcat(pcs->meta.szFile, p->szName);
			strcat(pcs->history.szFile, p->szName);

			if (p->next) {
				b = isDir(pcs->data.szFile);
				if (!b) {
					mkdir(pcs->data.szFile, 0777);
				}

				b = isDir(pcs->meta.szFile);
				if (!b) {
					mkdir(pcs->meta.szFile, 0777);
				}

				b = isDir(pcs->history.szFile);
				if (!b) {
					mkdir(pcs->history.szFile, 0777);
				}
			} else {
				/*
				 * For now, there is only .flh files,
				 * this could change.  E.g., the mid
				 * to name translation file in the history.
				 */
				strcat(pcs->data.szFile, ".flh");
				strcat(pcs->meta.szFile, ".flh");
				strcat(pcs->history.szFile, ".flh");

				b = isFile(pcs->data.szFile);

				/*
				 * Already exists!
				 */
				if (b) {
					freePathStackOf(stack);
					stack = (PathStackStruct *)NULL;
					continue;
				}

				b = isFile(pcs->meta.szFile);
				if (b) {
					/*
					 * Assume a race condition
					 * and reject this one.
					 */
					freePathStackOf(stack);
					stack = (PathStackStruct *)NULL;
					continue;
				}

				/*
				 * We don't care if the history
				 * file exists or not.  If it does,
				 * it is not an error.
				 */
				bFound = true;
				break;
			}
		}

		freePathStackOf(stack);
		stack = (PathStackStruct *)NULL;

		if (bFound) {
			break;
		}
	}

	eh_ASSERT(!stack);

	if (j == jBound) {
		fprintf(stderr, "No files to create\n");

		return(false);
	}

	printf("Going to create %s\n", pcs->data.szFile);
	pcs->data.bFullRange = true;
	pcs->data.bByteLocks = false;
	pcs->data.bWait = false;
	pcs->data.bCreate = true;
	pcs->data.bAppend = false;
	pcs->data.access = amRW;
	pcs->data.shareMode = smAll;

	if (eh_OpenFile(&pcs->data) == INVALID_HANDLE_VALUE) {
		if (pcs->data.iOpenFailures++ > 60) {
			eh_fatal("%s(%d): Could not get "
				"any creates off of %s\n",
				__FILE__, __LINE__, pcs->data.szBase);
		}

		return(false);
	} else {
		pcs->data.iOpenFailures = 0;
	}

	pcs->meta.bFullRange = true;
	pcs->meta.bByteLocks = false;
	pcs->meta.bWait = false;
	pcs->meta.bCreate = true;
	pcs->meta.bAppend = false;
	pcs->meta.access = amRW;
	pcs->meta.shareMode = smAll;

	if (eh_OpenFile(&pcs->meta) == INVALID_HANDLE_VALUE) {
		if (pcs->meta.iOpenFailures++ > 60) {
			eh_fatal("%s(%d): Could not get "
				"any creates off of %s\n",
				__FILE__, __LINE__, pcs->meta.szBase);
		}

		return(false);
	} else {
		pcs->meta.iOpenFailures = 0;
	}

	pcs->dfcb.signature = METAFILE_SIGNATURE;
	pcs->dfcb.version = METAFILE_VERSION;
	if (pcs->ds.rollBlocks) {
		pcs->dfcb.blockSize = ROLL_RANGE(pcs->ds.minbsize,
						pcs->ds.maxbsize,
						uint8, false);
	} else {
		pcs->dfcb.blockSize = pcs->ds.blockSize;
	}

	pcs->dfcb.fileSize = ROLL_RANGE(pcs->ds.minfsize,
				pcs->ds.maxfsize,
				uint8, false);

	/*
	 * Align such that we have an integer number of blocks.
	 */
	pcs->dfcb.blocks = pcs->dfcb.fileSize / pcs->dfcb.blockSize;
	if (pcs->dfcb.fileSize % pcs->dfcb.blockSize) {
		pcs->dfcb.blocks++;
		pcs->dfcb.fileSize = pcs->dfcb.blocks * pcs->dfcb.blockSize;
	}

	pcs->dfcb.seed = genrand_int32(false); 
	pcs->dfcb.flips = 0;
	printDFCB(pcs->dfcb);

	pcs->rs.blockSize = pcs->dfcb.blockSize;

	pcs->rs.block = allocateBlockData(pcs->rs.blockSize,
						__FILE__, __LINE__);

	pcs->rs.cooked = allocateBlockData(pcs->rs.blockSize,
						__FILE__, __LINE__);

	pcs->shuffled = allocateShuffled(pcs->dfcb.blocks, __FILE__, __LINE__);

	eh_ASSERT(!(pcs->dfcb.fileSize % pcs->dfcb.blockSize));

	rw = writeDFCB(pcs->meta.fd, &pcs->dfcb);
	if (rw != sizeof(DataFileControlBlock)) {
	}

	phq = windHistory(pcs, __FILE__, __LINE__);

	gettimeofday(&phq->hs.time, NULL);
	phq->hs.seed = genrand_int32(false);
	phq->hs.action = aeCreate;
	phq->hs.corrupt = false;
	phq->hs.block = 0;
	phq->hs.flips = 0;
	phq->hs.pattern = 0xdeade4e5;
	phq->hs.blockSize = pcs->dfcb.blockSize;
	phq->hs.pid = pcs->pid;
	phq->hs.mid = 0xdeade4e5;

	for (j = 0; j < pcs->dfcb.blocks; j++) {
		pcs->bs.offset = j * pcs->dfcb.blockSize;
		pcs->bs.blockSize = pcs->dfcb.blockSize;  /* ??? */
		pcs->bs.flips = 0;
		pcs->bs.seed = genrand_int32(false);
		pcs->bs.createPid = pcs->pid;
		pcs->bs.modifyPid = pcs->bs.createPid;
		pcs->bs.readPid = pcs->bs.createPid;
		pcs->bs.accessPid = pcs->bs.createPid;
		gettimeofday(&pcs->bs.createTime, NULL);
		pcs->bs.modifyTime = pcs->bs.createTime;
		pcs->bs.readTime = pcs->bs.createTime;
		pcs->bs.accessTime = pcs->bs.createTime;

		pcs->bs.inUse = usVirgin;
		pcs->bs.corrupt = false;

		for (iPick = 0;
			iPick < pcs->dfcb.blocks && iPick < pcs->ds.acton;
			iPick++ ) {
			if (pcs->shuffled[iPick] == j) {
				pcs->bs.inUse = usActive;
				break;
			}
		}

		rw = writeBlock(pcs->meta.fd, &pcs->bs);
		if (rw != sizeof(BlockStruct)) {
		}

		if (pcs->bs.inUse == usActive) {
			b = generateTheRecord(pcs);
		}
	}

	return(true);
}

/*
 * Set pcs->block to hit a specific block to verify.
 */
static bool
actionRead (ControlStruct *pcs)
{
	bool		b = false;
	DirEntryStruct	*pds = (DirEntryStruct *)NULL;

	uint8		iPick;
	uint8		rw;

	pds = findActiveFile(pcs);
	if (!pds) {
		return(false);
	}

	printf("Reading %s.\n", pcs->szFile);

	pcs->data.bFullRange = true;
	pcs->data.bByteLocks = false;
	pcs->data.bWait = false;
	pcs->data.bCreate = false;
	pcs->data.bAppend = false;
	pcs->data.access = amRead;
	pcs->data.shareMode = smNone;
	snprintf(pcs->data.szFile, EH_PATH_MAX, "%s/%s",
			pcs->data.szBase, pcs->szFile);

	if (eh_OpenFile(&pcs->data) == INVALID_HANDLE_VALUE) {
		return(false);
	}

	pcs->meta.bFullRange = true;
	pcs->meta.bByteLocks = false;
	pcs->meta.bWait = false;
	pcs->meta.bCreate = false;
	pcs->meta.bAppend = false;
	pcs->meta.access = amRW;
	pcs->meta.shareMode = smAll;
	snprintf(pcs->meta.szFile, EH_PATH_MAX, "%s/%s",
			pcs->meta.szBase, pcs->szFile);

	if (eh_OpenFile(&pcs->meta) == INVALID_HANDLE_VALUE) {
		return(false);
	}

	snprintf(pcs->history.szFile, EH_PATH_MAX, "%s/%s",
		pcs->history.szBase, pcs->szFile);

	rw = readDFCB(pcs->meta.fd, &pcs->dfcb);
	if (rw != sizeof(DataFileControlBlock)) {
		return(false);
	}

	if (pcs->dfcb.signature != METAFILE_SIGNATURE) {
		eh_fatal("%s(%d): Metafile signature %#lx "
				"does not match %#lx\n",
				__FILE__, __LINE__,
				pcs->dfcb.signature, METAFILE_SIGNATURE);
	}

	if (pcs->dfcb.version != METAFILE_VERSION) {
		eh_fatal("%s(%d): Metafile version %#lx "
				"does not match %#lx\n",
				__FILE__, __LINE__,
				pcs->dfcb.version, METAFILE_VERSION);
	}

	eh_ASSERT(!(pcs->dfcb.fileSize % pcs->dfcb.blockSize));

        /*
         * XXX: Do some more verifications!
         */

	printDFCB(pcs->dfcb);

	pcs->rs.blockSize = pcs->dfcb.blockSize;

	pcs->rs.block = allocateBlockData(pcs->rs.blockSize,
						__FILE__, __LINE__);

	pcs->rs.cooked = allocateBlockData(pcs->rs.blockSize,
						__FILE__, __LINE__);

	pcs->shuffled = allocateShuffled(pcs->dfcb.blocks, __FILE__, __LINE__);
	for (iPick = 0;
		iPick < pcs->dfcb.blocks && iPick < pcs->ds.acton;
		iPick++ ) {
		pcs->hitThisBlock = pcs->shuffled[iPick];
		b = verifyTheRecord(pcs);
		if (b) {
			break;
		}
	}

	return(true);
}

/*
 * At some point we will want to split write and delete into
 * two different functions.  It would be nice to detect that
 * we are deleting from the end of a file, and hence shrink it
 * down.  Might still fit within this common context.
 */
static bool
actionModify (ControlStruct *pcs)
{
	bool		b = false;
	DirEntryStruct	*pds = (DirEntryStruct *)NULL;

	uint8		iPick;
	uint8		rw;

	pds = findActiveFile(pcs);
	if (!pds) {
		return(false);
	}

	if (pcs->takeAction == aeWrite) {
		printf("Writing records to %s\n", pcs->szFile);
	} else {
		printf("Deleting records to %s\n", pcs->szFile);
	}

	pcs->data.bFullRange = true;
	pcs->data.bByteLocks = false;
	pcs->data.bWait = false;
	pcs->data.bCreate = false;
	pcs->data.bAppend = false;
	pcs->data.access = amRW;
	pcs->data.shareMode = smNone;
	snprintf(pcs->data.szFile, EH_PATH_MAX, "%s/%s",
			pcs->data.szBase, pcs->szFile);

	if (eh_OpenFile(&pcs->data) == INVALID_HANDLE_VALUE) {
		return(false);
	}

	pcs->meta.bFullRange = true;
	pcs->meta.bByteLocks = false;
	pcs->meta.bWait = false;
	pcs->meta.bCreate = false;
	pcs->meta.bAppend = false;
	pcs->meta.access = amRW;
	pcs->meta.shareMode = smAll;
	snprintf(pcs->meta.szFile, EH_PATH_MAX, "%s/%s",
			pcs->meta.szBase, pcs->szFile);

	if (eh_OpenFile(&pcs->meta) == INVALID_HANDLE_VALUE) {
		return(false);
	}

	snprintf(pcs->history.szFile, EH_PATH_MAX, "%s/%s",
		pcs->history.szBase, pcs->szFile);

	rw = readDFCB(pcs->meta.fd, &pcs->dfcb);
	if (rw != sizeof(DataFileControlBlock)) {
		return(false);
	}

	if (pcs->dfcb.signature != METAFILE_SIGNATURE) {
		eh_fatal("%s(%d): Metafile signature %#lx "
				"does not match %#lx\n",
				__FILE__, __LINE__,
				pcs->dfcb.signature, METAFILE_SIGNATURE);
	}

	if (pcs->dfcb.version != METAFILE_VERSION) {
		eh_fatal("%s(%d): Metafile version %#lx "
				"does not match %#lx\n",
				__FILE__, __LINE__,
				pcs->dfcb.version, METAFILE_VERSION);
	}

	eh_ASSERT(!(pcs->dfcb.fileSize % pcs->dfcb.blockSize));

        /*
         * XXX: Do some more verifications!
         */

	printDFCB(pcs->dfcb);

	pcs->rs.blockSize = pcs->dfcb.blockSize;

	pcs->rs.block = allocateBlockData(pcs->rs.blockSize,
						__FILE__, __LINE__);

	pcs->rs.cooked = allocateBlockData(pcs->rs.blockSize,
						__FILE__, __LINE__);

	pcs->shuffled = allocateShuffled(pcs->dfcb.blocks, __FILE__, __LINE__);
	for (iPick = 0;
		iPick < pcs->dfcb.blocks && iPick < pcs->ds.acton;
		iPick++ ) {
		pcs->hitThisBlock = pcs->shuffled[iPick];
		b = verifyTheRecord(pcs);
		if (!b) {
			break;
		}

		/*
		 * Data is verified, so now we need to go ahead
		 * and generate a new block.
		 */
		b = generateTheRecord(pcs);
	}

	return(true);
}

static bool
actionUnlink (ControlStruct *pcs)
{
	uint8		rw;

	bool		b = false;
	DirEntryStruct	*pds = (DirEntryStruct *)NULL;

	pds = findActiveFile(pcs);
	if (!pds) {
		return(false);
	}

	printf("Unlinking %s.\n", pcs->szFile);

	pcs->data.bFullRange = true;
	pcs->data.bByteLocks = false;
	pcs->data.bWait = false;
	pcs->data.bCreate = false;
	pcs->data.bAppend = false;
	pcs->data.access = amRW;
	pcs->data.shareMode = smAll;
	snprintf(pcs->data.szFile, EH_PATH_MAX, "%s/%s",
			pcs->data.szBase, pcs->szFile);

	if (eh_OpenFile(&pcs->data) == INVALID_HANDLE_VALUE) {
		return(false);
	}

	pcs->meta.bFullRange = true;
	pcs->meta.bByteLocks = false;
	pcs->meta.bWait = false;
	pcs->meta.bCreate = false;
	pcs->meta.bAppend = false;
	pcs->meta.access = amRW;
	pcs->meta.shareMode = smAll;
	snprintf(pcs->meta.szFile, EH_PATH_MAX, "%s/%s",
			pcs->meta.szBase, pcs->szFile);

	if (eh_OpenFile(&pcs->meta) == INVALID_HANDLE_VALUE) {
		return(false);
	}

	snprintf(pcs->history.szFile, EH_PATH_MAX, "%s/%s",
		pcs->history.szBase, pcs->szFile);

	rw = readDFCB(pcs->meta.fd, &pcs->dfcb);
	if (rw != sizeof(DataFileControlBlock)) {
		return(false);
	}

	if (pcs->dfcb.signature != METAFILE_SIGNATURE) {
		eh_fatal("%s(%d): Metafile signature %#lx "
				"does not match %#lx\n",
				__FILE__, __LINE__,
				pcs->dfcb.signature, METAFILE_SIGNATURE);
	}

	if (pcs->dfcb.version != METAFILE_VERSION) {
		eh_fatal("%s(%d): Metafile version %#lx "
				"does not match %#lx\n",
				__FILE__, __LINE__,
				pcs->dfcb.version, METAFILE_VERSION);
	}

	eh_ASSERT(!(pcs->dfcb.fileSize % pcs->dfcb.blockSize));

        /*
         * XXX: Do some more verifications!
         */

	printDFCB(pcs->dfcb);

	pcs->rs.blockSize = pcs->dfcb.blockSize;

	pcs->rs.block = allocateBlockData(pcs->rs.blockSize,
						__FILE__, __LINE__);

	pcs->rs.cooked = allocateBlockData(pcs->rs.blockSize,
						__FILE__, __LINE__);

	for (pcs->hitThisBlock = 0;
		pcs->hitThisBlock < pcs->dfcb.blocks;
		pcs->hitThisBlock++ ) {

		b = verifyTheRecord(pcs);
		if (!b) {
			break;
		}
	}

	/*
	 * Data has been verified, so now we delete the
	 * meta and data files, but not the history file.
	 */
	unlink(pcs->data.szFile);
	eh_CloseFile(&pcs->data);

	unlink(pcs->meta.szFile);
	eh_CloseFile(&pcs->meta);

#if 0
	printf("Sleeping now, please verify that both %s and %s "
		"are gone away, far away!\n", pcs->data.szFile,
		pcs->meta.szFile);

	sleep(60);
#endif

	return(true);
}
