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
 * Link list functionality for manipulating directory entries
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#include "elham.h"

/*
 * All the functions hide the workings of the linked list. The main does not
 * know the format of the list.
 */

bool
isEmptyList(HeadStruct *phs)
{
	eh_ASSERT(phs != NULL);

	if (phs->pdsHead || phs->pdsTail) {
		return(false);
	}

	return(true);
}

void
destroyList(HeadStruct *phs)
{
	DirEntryStruct     *pds = (DirEntryStruct *)NULL;
	DirEntryStruct     *pdsNext = (DirEntryStruct *)NULL;

	eh_ASSERT(phs != NULL);

	if (isEmptyList(phs)) {
		return;
	}
	for (pds = phs->pdsHead; pds != NULL; pds = pdsNext) {
		pdsNext = pds->pdsNext;

#if defined (TEST_DEBUG)
		pds->pdsPrev = (DirEntryStruct *) NULL;
		pds->pdsNext = (DirEntryStruct *) NULL;
#endif

		/*
		 * If the caller wants to save sublists, then
		 * they need to steal the HeadStruct.
		 */
		if (pds->phs) {
			destroyList(pds->phs);
			free(pds->phs);
		}

		free(pds);
	}

	phs->pdsHead = (DirEntryStruct *) NULL;
	phs->pdsTail = (DirEntryStruct *) NULL;
}

/*
 * If only one element, then return NULL.
 */
DirEntryStruct     *
getNextElementOf(HeadStruct *phs, uint8 uId)
{
	DirEntryStruct     *pds = (DirEntryStruct *)NULL;

	eh_ASSERT(phs != NULL);

	if (isEmptyList(phs)) {
		return (NULL);
	}
	for (pds = phs->pdsHead; pds != NULL; pds = pds->pdsNext) {
		if (pds->uId == uId) {
			return (pds->pdsNext);
		}
	}

	return (NULL);
}

void
appendElement(HeadStruct *phs, DirEntryStruct *pds)
{
	eh_ASSERT(phs != NULL);

	if (isEmptyList(phs)) {
		phs->pdsHead = pds;
		phs->pdsTail = pds;

		phs->uId = 0;
		phs->uDirs = 0;
		phs->uFiles = 0;
		phs->uSumDirs = 0;
		phs->uSumFiles = 0;
	} else {
		phs->pdsTail->pdsNext = pds;
		pds->pdsPrev = phs->pdsTail;
		phs->pdsTail = pds;
	}

	if (pds->bDir) {
		phs->uDirs++;
		phs->uSumDirs++;
	} else {
		phs->uFiles++;
		phs->uSumFiles++;
	}

	pds->uId = phs->uId;
	phs->uId++;
}

void
newList(HeadStruct *phs)
{
	eh_ASSERT(phs != NULL);

	phs->pdsHead = (DirEntryStruct *) NULL;
	phs->pdsTail = (DirEntryStruct *) NULL;
}

void
newHead(HeadStruct *phs)
{
	eh_ASSERT(phs != NULL);

	memset(phs, '\0', sizeof(HeadStruct));

	newList(phs);
}
