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
 * Play with Directory Entries
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>

#include "elham.h"

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

#if defined (unix)

bool
isDir (char *path)
{
        struct stat st;

	eh_ASSERT(path);

        if (stat(path, &st) == -1)
                return false;

        return S_ISDIR(st.st_mode);
}

bool
isFile (char *path)
{
	struct stat st;

	eh_ASSERT(path);
	
        if (stat(path, &st) == -1)
                return false;

        return S_ISREG(st.st_mode);
}

#endif

/*
 * Yes, this is grossly inefficient, even if the client is caching
 * with a dnl scheme.  But the overall goal is to generate and stress
 * READDIR traffic, whether from CIFS, PCNFS, or NFS.
 *
 * BTW: This algorithm is not inefficient, it is the way we use it
 * which would suck in a production environment.  See, we throw
 * away the results only to regenerate them right away.
 * 
 * Some claim might be made to capture new changes to the filesystem,
 * but really, we want the traffic.
 */
bool
getDirEntriesOf (HeadStruct *phs, DirTypeEnum dte, char *szBase)
{
	DirEntryStruct	*pds = (DirEntryStruct *)NULL;
	char		*psz = (char *)NULL;
	bool		bDir;
	bool		b;
	char	sz[EH_PATH_MAX];

#if !defined (unix)
#else
	DIR	*dp = (DIR *)NULL;
	struct dirent	*pde = (struct dirent *)NULL;
#endif

	eh_ASSERT(phs);
	if (!isEmptyList(phs)) {
		eh_fatal("%s(%d): Link list is full when it should be empty.\n",
			__FILE__, __LINE__);
	}

#if !defined (unix)
	
#else
	dp = opendir(szBase);
	if (!dp) {
		return(false);
	}

	while ((pde = readdir(dp)) != NULL) {
		snprintf(sz, EH_PATH_MAX, "%s/%s", szBase, pde->d_name);

		if (isDir(sz)) {
			if (strcmp(".", pde->d_name) == 0 ||
				strcmp(".snapshot", pde->d_name) == 0 ||
				strcmp("..", pde->d_name) == 0) {
				continue;
			}

			bDir = true;
		} else {
			bDir = false;
		}

		psz = strrchr(pde->d_name, '.');
		if (!psz) {
			continue;
		}

		psz++;

		/*
		 * Since all searches are on dteData, just flesh it
		 * out.  Add other file types if and when we decide
		 * on them.
		 */
		switch (dte) {
		case (dteMeta) :
		case (dteParity) :
		case (dteSums) :
		case (dteHistory) :

		case (dteData) :
			if (bDir) {
				if (STRNICMP(psz, "dlh", 3)) {
					continue;
				}
			} else {
				if (STRNICMP(psz, "flh", 3)) {
					continue;
				}
			}
			break;
		default :
			eh_fatal("%s(%d): Switch out of range - %d\n",
					__FILE__, __LINE__, dte);
			break;
		}

		pds = (DirEntryStruct *)calloc(1, sizeof(DirEntryStruct));
		if (!pds) {
			eh_fatal("%s(%d): Could not allocate %d bytes\n",
				__FILE__, __LINE__, sizeof(DirEntryStruct));
		}

		strncpy(pds->szName, sz, EH_PATH_MAX - 1);
		pds->bDir = bDir;

		appendElement(phs, pds);

		if (pds->bDir) {
			pds->phs = (HeadStruct *)calloc(1, sizeof(HeadStruct));
			if (!pds->phs) {
				eh_fatal("%s(%d): Could not allocate %d bytes\n",
					__FILE__, __LINE__, sizeof(HeadStruct));
			}

			newHead(pds->phs);

			b = getDirEntriesOf(pds->phs, dte, sz);
			if (!b) {
				return(false);
			}

			phs->uSumDirs += pds->phs->uSumDirs;
			phs->uSumFiles += pds->phs->uSumFiles;
		}
	}

	closedir(dp);
#endif

	return(true);
}

void
printDirEntriesOf (HeadStruct *phs, char *szBase)
{
	DirEntryStruct	*pds = (DirEntryStruct *)NULL;
#if 0
	char szPath[EH_PATH_MAX];
#endif

	eh_ASSERT(phs);

	printf("%s has "LLUFMT" direct files and "LLUFMT" direct dirs\n",
		szBase, phs->uFiles, phs->uDirs);
	printf("%s has "LLUFMT" descendant files and "
		LLUFMT" descendant dirs\n",
		szBase, phs->uSumFiles, phs->uSumDirs);

	if (isEmptyList(phs)) {
		return;
	}

	for (pds = phs->pdsHead; pds != NULL; pds = pds->pdsNext) {
		printf("%s is a %s\n", pds->szName,
			pds->bDir ? "directory" : "file");
#if 0
		newBaseVerifyDirEntryOf(szPath, pds->szName,
					phs->meta.szBase, phs->data.szBase,
					pds->bDir);
		printf("%s is the meta for %s\n", szPath, pds->szName);
#endif

		if (pds->bDir) {
			printDirEntriesOf(pds->phs, pds->szName);
		}
	}
}

/*
 * Find a valid file name and stuff the base part into
 * pcs->szFile.
 */
DirEntryStruct *
findActiveFile (ControlStruct *pcs)
{
	uint8		uToFind = 0;
	uint8		uCurr = 0;
	DirEntryStruct	*pds = (DirEntryStruct *)NULL;

	eh_ASSERT(pcs);

	if (pcs->data.hs.uSumFiles) {
		uToFind = ROLL_UINT8(pcs->data.hs.uSumFiles, false);
	}

	pds = findDirEntryOf(&pcs->data.hs, fdFile, uToFind, &uCurr);
	if (!pds) {
		pcs->szFile[0] = '\0';
		return(pds);
	}

	if (!pullBaseOutOf(pcs->szFile, pds->szName, pcs->data.szBase)) {
		return(NULL);
	}

	return(pds);
}

DirEntryStruct *
findDirEntryOf (HeadStruct *phs, FindDirEnum fd, uint8 uToFind, uint8 *puCurr)
{
	DirEntryStruct	*pds = (DirEntryStruct *)NULL;
	DirEntryStruct	*pdsSub = (DirEntryStruct *)NULL;

	eh_ASSERT(phs);
	eh_ASSERT(puCurr);

	if (isEmptyList(phs)) {
		return(NULL);
	}

	for (pds = phs->pdsHead; pds != NULL; pds = pds->pdsNext) {
		if (*puCurr > uToFind) {
			eh_fatal("%s(%d): Current out of bounds - "
				LLUFMT" >= "LLUFMT"\n",
					__FILE__, __LINE__, *puCurr, uToFind);
		}

		switch (fd) {
		case (fdFile) :
			if (!pds->bDir) {
				if (*puCurr == uToFind) {
					return(pds);
				}

				(*puCurr)++;
			}
			break;

		case (fdDir) :
			if (pds->bDir) {
				if (*puCurr == uToFind) {
					return(pds);
				}

				(*puCurr)++;
			}

			break;

		case (fdEither) :
			if (*puCurr == uToFind) {
				return(pds);
			}

			(*puCurr)++;

			break;

		default :
			eh_fatal("%s(%d): Switch out of range - %d\n",
					__FILE__, __LINE__, fd);
			break;
		}

		if (pds->bDir && pds->phs) {
			pdsSub = findDirEntryOf(pds->phs,
						fd, uToFind,
						puCurr);
			if (pdsSub) {
				return(pdsSub);
			}
		}
	}

	return(NULL);
}

bool
pullBaseOutOf (char *szFileName, char *szOldPath, char *szOldBase)
{
	int	i1;
	int	i2;

	eh_ASSERT(szFileName);
	eh_ASSERT(szOldPath);
	eh_ASSERT(szOldBase);

	i1 = strlen(szOldPath);
	i2 = strlen(szOldBase);

	if (i1 < i2) {
		eh_fatal("%s(%d): Substring:\n|%s| (%d) "
			"larger than superstring:\n|%s| (%d)\n",
			__FILE__, __LINE__, szOldBase, i2, szOldPath, i1);
	}

	/*
	 * Advance past seperator.
	 */
	strncpy(szFileName, &szOldPath[i2 + 1], EH_PATH_MAX - 1);

	return(true);
}

/*
 * Rewrite in light of pullBaseOutOf().
 */
bool
newBaseVerifyDirEntryOf (char *szPath, char *szOldPath,
			char *szBase, char *szOldBase, bool bDir)
{
	char	sz1[EH_PATH_MAX];
	char	sz2[EH_PATH_MAX];
	char	*psz = (char *)NULL;
	char	*pszEnd = (char *)NULL;

	char	c = DIR_SEP;

	int	i1;
	int	i2;

	eh_ASSERT(szPath);
	eh_ASSERT(szOldPath);
	eh_ASSERT(szBase);
	eh_ASSERT(szOldBase);

	i1 = strlen(szOldPath);
	i2 = strlen(szOldBase);

	if (i1 < i2) {
		eh_fatal("%s(%d): Substring:\n|%s| (%d) "
			"larger than superstring:\n|%s| (%d)\n",
			__FILE__, __LINE__, szOldBase, i2, szOldPath, i1);
	}

	snprintf(szPath, EH_PATH_MAX, "%s%s", szBase, &szOldPath[i2]);
	strcpy(sz1, szBase);
	strcpy(sz2, &szOldPath[i2+1]);

	psz = sz2;

	/*
	 * Verify all of the directories up until the last name.
	 */
	while ((pszEnd = strchr(psz, c)) != NULL) {
		*pszEnd = '\0';
		strcat(sz1, DIR_SEP_STR);
		strcat(sz1, psz);
		psz = pszEnd + 1;

		/*
		 * Unix only??
		 */
		if (!isDir(sz1)) {
			eh_fatal("%s(%d): Companion directory |%s| "
				"does not exist for |%s|\n"
				"\tCould not find path |%s|\n",
				__FILE__, __LINE__, szPath, szOldPath, sz1);
		}
	}

	if (bDir) {
		if (!isDir(szPath)) {
			eh_fatal("%s(%d): Companion directory |%s| "
				"does not exist for |%s|\n",
				__FILE__, __LINE__, szPath, szOldPath );
		}
	} else {
		if (!isFile(szPath)) {
			eh_fatal("%s(%d): Companion file |%s| "
				"does not exist for |%s|\n",
				__FILE__, __LINE__, szPath, szOldPath );
		}
	}

	return(true);
}
