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
 * State transition code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>

#include "elham.h"

#define HARD_EDGE_MIN  0.01
#define SOFT_EDGE_MIN  0.05
#define SOFT_EDGE_MAX  0.95
#define HARD_EDGE_MAX  0.99

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_STRUCT_STATVFS_T
#define STATFS statvfs
#define STATFS_t statvfs_t
#else
#define STATFS statfs
#define STATFS_t struct statfs
#endif

static void initLimitsOf (TriggerStruct *pts);

char *
stateStringOf (StateEnum st)
{
	switch (st) {
	case (stEmpty) :
		return("Empty");
	case (stSoftEmpty) :
		return("SoftEmpty");
	case (stHardEmpty) :
		return("HardEmpty");
	case (stSoftFull) :
		return("SoftFull");
	case (stHardFull) :
		return("HardFull");
	case (stFull) :
		return("Full");
	case (stNormal) :
		return("Normal");
	default :
		eh_fatal("%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, st);
		break;
	}

	return(NULL);
}

static void
rotateState (TriggerStruct *trig, StateEnum s)
{
	uint4	i;

	eh_ASSERT(trig);
	eh_ASSERT(trig->state);

	if (trig->state[0] == s) {
		trig->bRotate = false;
		return;
	}

	trig->bRotate = true;

	printf("State transition for %s: %s to %s\n",
		trig->name, stateStringOf(trig->state[0]),
		stateStringOf(s));
			
	for (i = trig->iMemory - 1; i > 0; i--) {
		trig->state[i] = trig->state[i - 1];
	}

	trig->state[0] = s;
}

static void
stateChangeOf (TriggerStruct *trig)
{
	eh_ASSERT(trig);
	eh_ASSERT(trig->state);

	if (trig->curr == trig->max.hard) {
		rotateState(trig, stFull);
	} else if (trig->curr >= trig->max.hard) {
		rotateState(trig, stHardFull);
	} else if (trig->curr >= trig->max.soft) {
		rotateState(trig, stSoftFull);
	} else if (trig->curr == trig->min.limit) {
		rotateState(trig, stEmpty);
	} else if (trig->curr <= trig->min.soft) {
		rotateState(trig, stSoftEmpty);
	} else if (trig->curr <= trig->min.hard) {
		rotateState(trig, stHardEmpty);
	} else {
		rotateState(trig, stNormal);
	}
}

static void
currFileSystem (NamelessStruct *pns, char *szBase)
{
	int		i;
	STATFS_t	sfs;

	eh_ASSERT(pns);
	eh_ASSERT(szBase);

	i = STATFS(szBase, &sfs);
	if (!i) {
		if (sfs.f_files != pns->files.max.limit) {
			pns->files.max.limit = sfs.f_files;
			initLimitsOf(&pns->files);
		}

		if (sfs.f_blocks != pns->blocks.max.limit) {
			pns->files.max.limit = sfs.f_blocks;
			initLimitsOf(&pns->blocks);
		}

		pns->files.curr = sfs.f_files - sfs.f_ffree;
		pns->blocks.curr = sfs.f_blocks - sfs.f_bavail;
	}
}

/*
 * Have implemented code for files.
 * Need rules for dirs and blocks.
 *
 * Space can be different for all 3 bases, whereas
 * files and dirs are the same.
 *
 * Space, of course, is the most important to implement.
 */
void
transitStates (ControlStruct *pcs)
{
	uint4		change;
	StateEnum	state;

	eh_ASSERT(pcs);
	eh_ASSERT(pcs->state.files.state);

	currFileSystem(&pcs->state.meta, pcs->meta.szBase);
	currFileSystem(&pcs->state.data, pcs->data.szBase);
	currFileSystem(&pcs->state.history, pcs->history.szBase);

	stateChangeOf(&pcs->state.dirs);
	stateChangeOf(&pcs->state.files);
	stateChangeOf(&pcs->state.meta.files);
	stateChangeOf(&pcs->state.meta.blocks);
	stateChangeOf(&pcs->state.data.files);
	stateChangeOf(&pcs->state.data.blocks);
	stateChangeOf(&pcs->state.history.files);
	stateChangeOf(&pcs->state.history.blocks);
	
	/*
	 * First get the logical file system
	 * limitations.
	 */
	if (pcs->state.files.bRotate || pcs->state.data.files.bRotate ||
		pcs->state.meta.files.bRotate ||
		pcs->state.history.files.bRotate) {

		/*
		 * Whichever is worst drives the state.
		 */
		state = MAX(pcs->state.files.state[0],
				MAX(pcs->state.data.files.state[0],
				MAX(pcs->state.meta.files.state[0],
					pcs->state.history.files.state[0])));

		switch (state) {

		case (stNormal) :

			pcs->actions.create = pcs->ds.actions.create;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = pcs->ds.actions.write;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = pcs->ds.actions.unlink;

			break;

		case (stEmpty) :

			pcs->actions.create = 100;
			pcs->actions.read = 0;
			pcs->actions.write = 0;
			pcs->actions.delete = 0;
			pcs->actions.unlink = 0;

			break;

		case (stHardEmpty) :

			pcs->actions.create = pcs->ds.actions.create +
						pcs->ds.actions.unlink;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = pcs->ds.actions.write;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = 0;

			break;

		case (stSoftEmpty) :

			change = pcs->ds.actions.unlink * 0.6;
			pcs->actions.create = pcs->ds.actions.create +
						change;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = pcs->ds.actions.write;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = pcs->ds.actions.unlink -
						change;

			break;

		case (stFull) :

			pcs->actions.create = 0;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = pcs->ds.actions.write;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = pcs->ds.actions.unlink +
						pcs->ds.actions.create;

			break;

		case (stHardFull) :

			change = pcs->ds.actions.create * 0.8;
			pcs->actions.create = pcs->ds.actions.create -
						change;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = pcs->ds.actions.write;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = pcs->ds.actions.unlink +
						change;

			break;

		case (stSoftFull) :

			change = pcs->ds.actions.create * 0.6;
			pcs->actions.create = pcs->ds.actions.create -
						change;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = pcs->ds.actions.write;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = pcs->ds.actions.unlink +
						change;

			break;

		default :
			eh_fatal("%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, 
				pcs->state.files.state[0]);
			break;
		}
	}

	/*
	 * Now apply the physical blocks constraints.
	 */
	if (pcs->state.history.blocks.bRotate) {
		switch (pcs->state.history.blocks.state[0]) {

		case (stNormal) :
		case (stEmpty) :
		case (stHardEmpty) :
		case (stSoftEmpty) :

			break;

		case (stFull) :

			eh_fatal("%s(%d): History storage is out of space - %s\n",
				__FILE__, __LINE__, pcs->history.szBase);
			break;

		case (stHardFull) :

			printf("Please *really* consider adding space to your"
				" history storage: %s\n", pcs->history.szBase);
			break;

		case (stSoftFull) :

			printf("Please consider adding space to your"
				" history storage: %s\n", pcs->history.szBase);
			break;

		default :
			eh_fatal("%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, 
				pcs->state.files.state[0]);
			break;
		}
	}

	if (pcs->state.meta.blocks.bRotate || pcs->state.data.blocks.bRotate) {

		/*
		 * Whichever is worst drives the state.
		 */
		state = MAX(pcs->state.meta.blocks.state[0],
				pcs->state.data.blocks.state[0]);

		switch (state) {

		case (stNormal) :

			pcs->actions.create = pcs->ds.actions.create;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = pcs->ds.actions.write;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = pcs->ds.actions.unlink;

			break;

		case (stEmpty) :

			pcs->actions.create = 100;
			pcs->actions.read = 0;
			pcs->actions.write = 0;
			pcs->actions.delete = 0;
			pcs->actions.unlink = 0;

			break;

		case (stHardEmpty) :

			pcs->actions.create = pcs->ds.actions.create +
						pcs->ds.actions.unlink;
			pcs->actions.read = 0;
			pcs->actions.write = pcs->ds.actions.write +
						pcs->ds.actions.delete +
						pcs->ds.actions.read;
			pcs->actions.delete = 0;
			pcs->actions.unlink = 0;

			break;

		case (stSoftEmpty) :

			change = pcs->ds.actions.unlink * 0.6;
			pcs->actions.create = pcs->ds.actions.create +
						change;
			pcs->actions.unlink = pcs->ds.actions.unlink -
						change;

			pcs->actions.read = pcs->ds.actions.read;

			change = pcs->ds.actions.delete * 0.6;
			pcs->actions.write = pcs->ds.actions.write +
						change;
			pcs->actions.delete = pcs->ds.actions.delete -
						change;

			break;

		case (stFull) :

			pcs->actions.create = 0;
			pcs->actions.read = 0;
			pcs->actions.write = 0;
			pcs->actions.delete = 0;
			pcs->actions.unlink = 100;

			break;

		case (stHardFull) :

			change = pcs->ds.actions.create * 0.8;
			pcs->actions.create = pcs->ds.actions.create -
						change;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = 0;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = pcs->ds.actions.unlink +
						pcs->ds.actions.write +
						change;

			break;

		case (stSoftFull) :

			change = pcs->ds.actions.create * 0.6;
			pcs->actions.create = pcs->ds.actions.create -
						change;
			pcs->actions.read = pcs->ds.actions.read;
			pcs->actions.write = 0;
			pcs->actions.delete = pcs->ds.actions.delete;
			pcs->actions.unlink = pcs->ds.actions.unlink +
						pcs->ds.actions.write +
						change;

			break;

		default :
			eh_fatal("%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, 
				pcs->state.files.state[0]);
			break;
		}
	}


	if (pcs->ds.scan == scRead) {
		/*
		 * If we are a read scanner, we can not take
		 * any other actions, so readjust back to
		 * the prototypical reader.
		 */
		pcs->actions.read = 100;
		pcs->actions.create = 0;
		pcs->actions.write = 0;
		pcs->actions.delete = 0;
		pcs->actions.unlink = 0;
	} else if (pcs->ds.scan == scWrite) {
		eh_ASSERT(pcs->actions.read == 0);
	}

	verifyActionWeightsOf(pcs->actions, __FILE__, __LINE__);
}

static void
freeTriggerOf (TriggerStruct *pts)
{
	eh_ASSERT(pts);

	if (pts->grades) {
		free(pts->grades);
		pts->grades = (uint8 *)NULL;
		pts->iGrades = 0;
	}

	if (pts->state) {
		free(pts->state);
		pts->state = (StateEnum *)NULL;
		pts->iMemory = 0;
	}
}

static void
freeStateOf (NamelessStruct *pns)
{
	eh_ASSERT(pns);

	freeTriggerOf(&pns->files);
	freeTriggerOf(&pns->blocks);
}

void
cleanupState (ControlStruct *pcs)
{
	eh_ASSERT(pcs);

	freeTriggerOf(&pcs->state.dirs);
	freeTriggerOf(&pcs->state.files);
	freeStateOf(&pcs->state.meta);
	freeStateOf(&pcs->state.data);
	freeStateOf(&pcs->state.history);

	return;
}

static void
initGradesOf (TriggerStruct *pts, int iGrades, char *szFile, int iLine)
{
	eh_ASSERT(pts);
	eh_ASSERT(szFile);

	pts->iGrades = iGrades;

	pts->grades = (uint8 *)calloc(pts->iGrades, sizeof(uint8));
	if (!pts->grades) {
		eh_fatal("%s(%d): Could not allocate %d bytes\n",
			szFile, iLine, pts->iGrades * sizeof(uint8));
	}
}

static void
initMemoryOf (TriggerStruct *pts, int iMemory, char *name, char *type,
		char *szFile, int iLine)
{
	eh_ASSERT(pts);
	eh_ASSERT(szFile);
	eh_ASSERT(name);

	pts->iMemory = iMemory;

	if (type) {
		snprintf(pts->name, 15, "%s_%s", name, type);
	} else {
		strncpy(pts->name, name, 15);
	}

	pts->state = (StateEnum *)calloc(pts->iMemory, sizeof(StateEnum));
	if (!pts->state) {
		eh_fatal("%s(%d): Could not allocate %d bytes\n",
			szFile, iLine, pts->iMemory * sizeof(StateEnum));
	}

	pts->state[0] = stNormal;
}

static void
initFileSystem (NamelessStruct *pns, char *szBase)
{
	int		i;
	STATFS_t	sfs;

	eh_ASSERT(pns);
	eh_ASSERT(szBase);

	pns->blocks.min.limit = 0;
	pns->blocks.max.limit = 1000000;

	pns->files.min.limit = 0;
	pns->files.max.limit = 60000;

	i = STATFS(szBase, &sfs);
	if (!i) {
		pns->files.max.limit = sfs.f_files;
		pns->blocks.max.limit = sfs.f_blocks;
	}
}

static void
initStateOf (NamelessStruct *pns, int iMemory, char *name,
		char *szFile, int iLine)
{
	eh_ASSERT(pns);
	eh_ASSERT(szFile);
	eh_ASSERT(name);

	initMemoryOf(&pns->files, iMemory, name, "files", szFile, iLine);
	initMemoryOf(&pns->blocks, iMemory, name, "blocks", szFile, iLine);
}

static void
initLimitsOf (TriggerStruct *pts)
{
	eh_ASSERT(pts);

	pts->min.hard = pts->min.limit + HARD_EDGE_MIN * pts->max.limit;
	pts->min.soft = pts->min.limit + SOFT_EDGE_MIN * pts->max.limit;
	pts->max.soft = SOFT_EDGE_MAX * pts->max.limit;
	pts->max.hard = HARD_EDGE_MAX * pts->max.limit;
}

static void
initLimits (NamelessStruct *pns)
{
	eh_ASSERT(pns);

	initLimitsOf(&pns->files);
	initLimitsOf(&pns->blocks);
}

void
initializeState (ControlStruct *pcs)
{
	uint8		i;
	uint8		u;

	eh_ASSERT(pcs);

	initMemoryOf(&pcs->state.dirs, 4, "dirs", NULL, __FILE__, __LINE__);
	initMemoryOf(&pcs->state.files, 4, "files", NULL, __FILE__, __LINE__);

	initStateOf(&pcs->state.meta, 4, "meta", __FILE__, __LINE__);
	initStateOf(&pcs->state.data, 4, "data", __FILE__, __LINE__);
	initStateOf(&pcs->state.history, 4, "history", __FILE__, __LINE__);

	initFileSystem(&pcs->state.meta, pcs->meta.szBase);
	initFileSystem(&pcs->state.data, pcs->data.szBase);
	initFileSystem(&pcs->state.history, pcs->history.szBase);

	/*
	 * Keep counts of sum number of X at a given depth.
	 * i.e., at depth N, we want the sum(X, M=0,N)
	 * This will allow us to quickly determine which level
	 * an X is on if randonly generated in the range of all X.
	 *
	 * We happen to be speaking "C" when we give
	 * the depth, so depth == 2 means 3 iterations.
	 */
	initGradesOf(&pcs->state.dirs,
			pcs->ds.depth + 1, __FILE__, __LINE__);
	initGradesOf(&pcs->state.files,
			pcs->ds.depth + 1, __FILE__, __LINE__);

	/*
	 * Cheap integer calculation to determine the max number of files.
	 */
	pcs->state.dirs.min.limit = 1;
	pcs->state.dirs.max.limit = 1;

	pcs->state.files.min.limit = 0;
	pcs->state.files.max.limit = pcs->ds.files;

	pcs->state.files.grades[0] = pcs->ds.files;
	pcs->state.dirs.grades[0] = 1;

	if (pcs->ds.depth != 0) {
		u = pcs->ds.width;

		for (i = 1; i < pcs->state.files.iGrades; i++) {
			pcs->state.dirs.grades[i] =
				pcs->state.dirs.grades[i - 1] + u;
			pcs->state.files.grades[i] =
				pcs->state.dirs.grades[i] * pcs->ds.files;
			u *= pcs->ds.width;
		}

		pcs->state.files.max.limit =
				pcs->state.files.grades[i - 1];
		pcs->state.dirs.max.limit =
				pcs->state.dirs.grades[i - 1];
	}

	initLimitsOf(&pcs->state.dirs);
	initLimitsOf(&pcs->state.files);

	initLimits(&pcs->state.meta);
	initLimits(&pcs->state.data);
	initLimits(&pcs->state.history);
}

/*
 * The heart of this algorithm is that we are trying to find a
 * node in the complete directory hierarchy and not the one
 * actually on disk.  This we can pretend that the tree is full
 * and get away with the math.
 */
PathStackStruct *
buildPathStackOf (ControlStruct *pcs, PathStackStruct *stack, FindDirEnum fd,
	uint8 uToFind, uint8 *puDepth)
{
	uint8		i;
	uint8		iId;
	uint8		iIndex;

	/*
	 * Sum contains a history of the number of X for
	 * X.grades[i - 1].  It gets updated at the end of
	 * the loop.
	 */
	uint8		sum = 0;

	/*
	 * iParent contains a history of the number of X for:
	 *
	 * files) dirs.grades[i - 1]
	 * dirs)  dirs.grades[i - 2]
	 *
	 * It is needed to find the id of the parent node.
	 */
	uint8		iParent = 0;

	PathStackStruct	*next = (PathStackStruct *)NULL;

	eh_ASSERT(pcs);
	eh_ASSERT(puDepth);
	eh_ASSERT(*puDepth != 0 || !stack);
	eh_ASSERT(*puDepth == 0 || stack);
	eh_ASSERT(*puDepth == 0 || fd != fdFile);
	eh_ASSERT(*puDepth != 0 || fd != fdDir);

	switch (fd) {
	case (fdFile) :

		/*
		 * Let iParent maintain the state of the sum
		 * for dirs for dirs.grade[i - 1].
		 */
		for (i = 0; i < pcs->state.files.iGrades; i++) {
			if (uToFind < pcs->state.files.grades[i]) {
				/*
				 * Now find out which dir it is in.
				 */
				iIndex = uToFind - sum;

				iParent += iIndex / pcs->ds.files;
				iId = iIndex % pcs->ds.files;

				next = (PathStackStruct *)calloc(1,
						sizeof(PathStackStruct));
				if (!next) {
					eh_fatal("%s(%d): Could not allocate "
						"%d bytes\n", __FILE__,
						__LINE__,
						sizeof(PathStackStruct));
				}

				next->depth = (*puDepth)++;
				snprintf(next->szName, EH_PATH_MAX,
					"%c"LLUFMT, DIR_SEP, iId);

				return(buildPathStackOf(pcs, next, fdDir,
					iParent, puDepth));
			}

			sum = pcs->state.files.grades[i];
			iParent = pcs->state.dirs.grades[i];
		}

		break;

	case (fdDir) :

		/*
		 * Base case for recursion.
		 */
		if (uToFind == 0) {
			return(stack);
		}

		for (i = 0; i < pcs->state.dirs.iGrades; i++) {
			if (uToFind < pcs->state.dirs.grades[i]) {
				/*
				 * Now find out which dir it is in.
				 */
				iIndex = uToFind - sum;

				iParent += iIndex / pcs->ds.width;
				iId = iIndex % pcs->ds.width;

				next = (PathStackStruct *)calloc(1,
						sizeof(PathStackStruct));
				if (!next) {
					eh_fatal("%s(%d): Could not allocate "
						"%d bytes\n", __FILE__,
						__LINE__,
						sizeof(PathStackStruct));
				}

				next->depth = (*puDepth)++;
				snprintf(next->szName, EH_PATH_MAX,
					"%c"LLUFMT".dlh", DIR_SEP, iId);

				if (stack) {
					next->next = stack;
					eh_ASSERT(next->depth ==
						(stack->depth + 1));
				}

				return(buildPathStackOf(pcs, next, fdDir,
					iParent, puDepth));
			}

			iParent = sum;
			sum = pcs->state.dirs.grades[i];
		}

		break;

	case (fdEither) :

#if 0
		break;
#endif

	default :
		eh_fatal("%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, fd);
		break;
	}

	return(NULL);
}

void
freePathStackOf (PathStackStruct *stack)
{
	PathStackStruct	*p = (PathStackStruct *)NULL;

	while (stack) {
		p = stack->next;
		free(stack);
		stack = p;
	}
}
