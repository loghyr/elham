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

## @summary Multi-Protocol data verifier
## @pod here
## @author thomas@netapp.com

=head1 Preamble

The elham utility, lock manager - hammer, is to provide both
multiprotocol lock manager testing and corruption testing.

=cut
*/

/*
 * Main for the lock manager testing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>

#include "elham.h"

#define VERSION "$Id$\n"
#define USAGE                                                                \
  "\nUsage: elham [OPTION ..]\n"                                             \
  "Version: "VERSION"\n"                                                     \
  "  -data PATH              Base path for data files\n"                     \
  "  -history PATH           Base path for history files\n"                  \
  "  -meta PATH              Base path for metafile files\n"                 \
  "  -seed N                 set initial random seed to N\n"                 \
  "  -flips N                number of times to invoke random to start\n"    \
  "  -lock                   Use defaults for lock manager testing\n"        \
  "  -hammer                 Use defaults for hammer testing\n"              \
  "   (The hammer and lock options are mutually exclusive.)\n"               \
  "  -width N                Maximum number of interior branches\n"          \
  "  -depth N                Maximum depth of directory tree\n"              \
  "  -files N                Maximum number of files in a directory\n"       \
  "  -iters N                Number of different actions to take\n"          \
  "  -acton N                Number of blocks to process for an action\n"    \
  "  -scan read              Always read files only\n"                       \
  "  -scan write             Always write files only\n"                      \
  "  -scan random            Randomly read/write to files\n"                 \
  "  -wait always            Always wait on locks\n"                         \
  "  -wait never             Never wait on locks\n"                          \
  "  -wait random            Randomly decide to wait on locks\n"             \
  "\n"                                                                       \
  "  -wtcreate N             %% weight to spend on file CREATES\n"           \
  "  -wtread N               %% weight to spend on block READS\n"            \
  "  -wtwrite N              %% weight to spend on block WRITES\n"           \
  "  -wtdelete N             %% weight to spend on block DELETES\n"          \
  "  -wtunlink N             %% weight to spend on file UNLINKS\n"           \
  "   (The sum of -wt* must be 100.)\n"                                      \
  "   (-wt* and -scan are mutually exclusive.)\n"                            \
  "\n"                                                                       \
  "  -blocksize NBYTES       use blockSize NBYTES\n"                         \
  "  -blocksize random       use a random blockSize\n"                       \
  "  -minbsize NBYTES        set minimum block size to NBYTES\n"             \
  "  -maxbsize NBYTES        set maximum block size to NBYTES\n"             \
  "  -minfsize NBYTES        set minimum file  size to NBYTES\n"             \
  "  -maxfsize NBYTES        set maximum file  size to NBYTES\n"             \
  "  -doc                    print parameters at end of processing\n"        \
  "  -version                print elham version\n"                          \
  "  -help                   print this list\n"                         

/*
 * Evil, but useful macro for making screen layout, but not
 * groking, okay.
 */
#if 0
#define TAKE_IF_CHANGED(f) \
	if (pcs->dsSupplied.##f != pcs->dsDefault.##f) { \
		pcs->ds.##f = pcs->dsSupplied.##f; \
	}
#else
#define TAKE_IF_CHANGED(f) \
	if (pcs->dsSupplied.f != pcs->dsDefault.f) { \
		pcs->ds.f = pcs->dsSupplied.f; \
	}
#endif

static void 
processArgs (ControlStruct *pcs, int argc, char *argv[])
{
	bool	bScanSet = false;
	bool	bWeightSet = false;
	bool	bDoc = false;

	strcpy(pcs->data.szBase, "./data");
	strcpy(pcs->meta.szBase, "./meta");
	strcpy(pcs->history.szBase, "./history");

	strcpy(pcs->data.szBase, "/t/mana/data");
	strcpy(pcs->meta.szBase, "/t/mana/meta");
	strcpy(pcs->history.szBase, "/t/mana/history");

	/*
	 * Fill in dsSupplied by what is supplied on the command line.
	 */
	for (NEXT_ARG(); argc > 0; NEXT_ARG()) {
		if (!strcmp(*argv, "-seed")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.seed = atol(*argv);
		} else if (!strcmp(*argv, "-flips")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.flips = atoll(*argv);
		} else if (!strcmp(*argv, "-minfsize")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.minfsize = atoll(*argv);
		} else if (!strcmp(*argv, "-maxfsize")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.maxfsize = atoll(*argv);
		} else if (!strcmp(*argv, "-minbsize")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.minbsize = atoll(*argv);
		} else if (!strcmp(*argv, "-maxbsize")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.maxbsize = atoll(*argv);
		} else if (!strcmp(*argv, "-blocksize")) {
			NEXT_ARG();
			NEED_ARG();
			if (!strcmp(*argv, "random")) {
				pcs->dsSupplied.rollBlocks = true;
			} else {
				pcs->dsSupplied.blockSize = atoll(*argv);
				pcs->dsSupplied.rollBlocks = false;
			}
		} else if (!strcmp(*argv, "-width")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.width = atoll(*argv);
		} else if (!strcmp(*argv, "-depth")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.depth = atoll(*argv);
		} else if (!strcmp(*argv, "-files")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.files = atoll(*argv);
		} else if (!strcmp(*argv, "-iters")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.iters = atoll(*argv);
		} else if (!strcmp(*argv, "-acton")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->dsSupplied.acton = atoll(*argv);
		} else if (!strcmp(*argv, "-data")) {
			NEXT_ARG();
			NEED_ARG();
			strncpy(pcs->data.szBase, *argv, EH_PATH_MAX);
		} else if (!strcmp(*argv, "-meta")) {
			NEXT_ARG();
			NEED_ARG();
			strncpy(pcs->meta.szBase, *argv, EH_PATH_MAX);
		} else if (!strcmp(*argv, "-history")) {
			NEXT_ARG();
			NEED_ARG();
			strncpy(pcs->history.szBase, *argv, EH_PATH_MAX);
		} else if (!strcmp(*argv, "-hammer")) {
			if (pcs->defaultTo != deDefault) {
				eh_fatal("Did you mean to set lock|"
					"hammer more than once?\n");
			}
			pcs->defaultTo = deHammer;
		} else if (!strcmp(*argv, "-lock")) {
			if (pcs->defaultTo != deDefault) {
				eh_fatal("Did you mean to set lock|"
					"hammer more than once?\n");
			}
			pcs->defaultTo = deLock;
		} else if (!strcmp(*argv, "-scan")) {
			NEXT_ARG();
			NEED_ARG();

			if (bWeightSet) {
				eh_fatal("You can't set -scan once you "
					"have set a -wt*\n");
			} else if (bScanSet) {
				eh_fatal("You can't set -scan twice\n");
			}

			bScanSet = true;

			if (!strcmp(*argv, "read")) {
				pcs->dsSupplied.scan = scRead;
			} else if (!strcmp(*argv, "write")) {
				pcs->dsSupplied.scan = scWrite;
			} else if (!strcmp(*argv, "random")) {
				pcs->dsSupplied.scan = scRandom;
			} else {
				eh_fatal("-scan does not accept %s\n"USAGE,
					*argv);
			}
		} else if (!strcmp(*argv, "-wt")) {
			if (bScanSet) {
				eh_fatal("You can't set %s once you have "
					"set -scan\n", *argv);
			}

			bWeightSet = true;

			if (!strcmp(*argv, "-wtcreate")) {
				NEXT_ARG();
				NEED_ARG();

				pcs->dsSupplied.actions.create = atoi(*argv);
			} else if (!strcmp(*argv, "-wtread")) {
				NEXT_ARG();
				NEED_ARG();

				pcs->dsSupplied.actions.read = atoi(*argv);
			} else if (!strcmp(*argv, "-wtwrite")) {
				NEXT_ARG();
				NEED_ARG();

				pcs->dsSupplied.actions.write = atoi(*argv);
			} else if (!strcmp(*argv, "-wtdelete")) {
				NEXT_ARG();
				NEED_ARG();

				pcs->dsSupplied.actions.delete = atoi(*argv);
			} else if (!strcmp(*argv, "-wtunlink")) {
				NEXT_ARG();
				NEED_ARG();

				pcs->dsSupplied.actions.unlink = atoi(*argv);
			} else {
				eh_fatal("Do not understand %s\n"USAGE, *argv);
			}
		} else if (!strcmp(*argv, "-wait")) {
			NEXT_ARG();
			NEED_ARG();
			if (!strcmp(*argv, "always")) {
				pcs->dsSupplied.wait = weAlways;
			} else if (!strcmp(*argv, "never")) {
				pcs->dsSupplied.wait = weNever;
			} else if (!strcmp(*argv, "random")) {
				pcs->dsSupplied.wait = weRandom;
			} else {
				eh_fatal("-wait does not accept %s\n"USAGE,
					*argv);
			}
		} else if (!strcmp(*argv, "-version")) {
			printf(VERSION);
			exit(0);
		} else if (!strcmp(*argv, "-help")) {
			printf(USAGE);
			exit(0);
		} else if (!strcmp(*argv, "-doc")) {
			bDoc = true;
		} else {
			eh_fatal("Do not understand %s\n"USAGE, *argv);
		}
	}

	/*
	 * Now apply the proper default based on whether we are stressing
	 * locks or corruption or neither the most.
	 */
	switch (pcs->defaultTo) {
	case (deDefault) :
		defaultStructCopy(&pcs->ds, &pcs->dsSupplied);
		break;
	case (deLock) :
		defaultStructCopy(&pcs->ds, &pcs->dsLock);
		break;
	case (deHammer) :
		defaultStructCopy(&pcs->ds, &pcs->dsHammer);
		break;
	default :
		eh_fatal("%s(%d): Switch out of range - %d\n",
			__FILE__, __LINE__, pcs->defaultTo);
		break;
	}

	/*
	 * Now lay the supplied changes on top of the proper defaults.
	 */
	TAKE_IF_CHANGED(minfsize);
	TAKE_IF_CHANGED(maxfsize);
	TAKE_IF_CHANGED(minbsize);
	TAKE_IF_CHANGED(maxbsize);
	TAKE_IF_CHANGED(blockSize);
	TAKE_IF_CHANGED(seed);
	TAKE_IF_CHANGED(flips);
	TAKE_IF_CHANGED(width);
	TAKE_IF_CHANGED(depth);
	TAKE_IF_CHANGED(acton);
	TAKE_IF_CHANGED(files);
	TAKE_IF_CHANGED(iters);
	TAKE_IF_CHANGED(scan);
	TAKE_IF_CHANGED(wait);
	TAKE_IF_CHANGED(rollBlocks);
	TAKE_IF_CHANGED(actions.create);
	TAKE_IF_CHANGED(actions.read);
	TAKE_IF_CHANGED(actions.write);
	TAKE_IF_CHANGED(actions.delete);
	TAKE_IF_CHANGED(actions.unlink);

	/*
	 * Determine the wt* and also validate them.
	 */
	if (pcs->ds.scan == scRead) {
		pcs->ds.actions.read = 100;
		pcs->ds.actions.create = 0;
		pcs->ds.actions.write = 0;
		pcs->ds.actions.delete = 0;
		pcs->ds.actions.unlink = 0;
	} else if (pcs->ds.scan == scWrite) {
		pcs->ds.actions.write += pcs->ds.actions.read;
		pcs->ds.actions.read = 0;
	}

	verifyActionWeightsOf(pcs->ds.actions, __FILE__, __LINE__);

	if (bDoc) {
		printf("=== Final Input Parameters ===\n");
		printf(DOCFMT" is %s\n", "-data", pcs->data.szBase);
		printf(DOCFMT" is %s\n", "-meta", pcs->meta.szBase);
		printf(DOCFMT" is %s\n", "-history", pcs->history.szBase);
		printf(DOCFMT" is "LLUFMT"\n", "-minfsize", pcs->ds.minfsize);
		printf(DOCFMT" is "LLUFMT"\n", "-maxfsize", pcs->ds.maxfsize);
		printf(DOCFMT" is "LLUFMT"\n", "-minbsize", pcs->ds.minbsize);
		printf(DOCFMT" is "LLUFMT"\n", "-maxbsize", pcs->ds.maxbsize);

		if (pcs->ds.rollBlocks) {
			printf(DOCFMT" is random\n", "-blocksize");
		} else {
			printf(DOCFMT" is "LLUFMT"\n",
				"-blocksize", pcs->ds.blockSize);
		}

		printf(DOCFMT" is %#lx\n", "-seed", pcs->ds.seed);
		printf(DOCFMT" is "LLUFMT"\n", "-flips", pcs->ds.flips);
		printf(DOCFMT" is "LLUFMT"\n", "-width", pcs->ds.width);
		printf(DOCFMT" is "LLUFMT"\n", "-depth", pcs->ds.depth);
		printf(DOCFMT" is "LLUFMT"\n", "-acton", pcs->ds.acton);
		printf(DOCFMT" is "LLUFMT"\n", "-files", pcs->ds.files);
		printf(DOCFMT" is "LLUFMT"\n", "-iters", pcs->ds.iters);

		printf(DOCFMT" is ", "-scan");
		switch (pcs->ds.scan) {
		case (scRead) :
			printf("read\n");
			break;
		case (scWrite) :
			printf("write\n");
			break;
		case (scRandom) :
			printf("random\n");
			break;
		default :
			eh_fatal("\n%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, pcs->ds.scan);
			break;
		}

		printf(DOCFMT" is ", "-wait");
		switch (pcs->ds.wait) {
		case (weAlways) :
			printf("always\n");
			break;
		case (weNever) :
			printf("never\n");
			break;
		case (weRandom) :
			printf("random\n");
			break;
		default :
			eh_fatal("\n%s(%d): Switch out of range - %d\n",
				__FILE__, __LINE__, pcs->ds.wait);
			break;
		}

		printf(DOCFMT" is %ld\n", "-wtread", pcs->ds.actions.read);
		printf(DOCFMT" is %ld\n", "-wtcreate", pcs->ds.actions.create);
		printf(DOCFMT" is %ld\n", "-wtwrite", pcs->ds.actions.write);
		printf(DOCFMT" is %ld\n", "-wtdelete", pcs->ds.actions.delete);
		printf(DOCFMT" is %ld\n", "-wtunlink", pcs->ds.actions.unlink);

		printf("=== Final Input Parameters ===\n");
	}

	pcs->actions.create = pcs->ds.actions.create;
	pcs->actions.read = pcs->ds.actions.read;
	pcs->actions.write = pcs->ds.actions.write;
	pcs->actions.delete = pcs->ds.actions.delete;
	pcs->actions.unlink = pcs->ds.actions.unlink;

	return;
}

int 
main (int argc, char *argv[])
{
	uint8		i;
	uint4		iTooManyNonDirs = 0;
	uint4		iTooManyNonFiles = 0;
	bool		bIter = false;

	ControlStruct	cs;

	initControlStruct(&cs);

#if defined (unix)
	cs.pid = getpid();
#else
	cs.pid = _getpid();
#endif

	printf("My pid is %ld\n", cs.pid);

	newHead(&cs.data.hs);
	newHead(&cs.meta.hs);
	newHead(&cs.history.hs);

	processArgs(&cs, argc, argv);

	verifyControlStruct(&cs);

	printf("My seed is %#lx\n", cs.ds.seed);

	init_genrand(cs.ds.seed, false);
	if (cs.ds.flips) {
		advance_flips(cs.ds.flips, false);
	}

	initializeState(&cs);

	if (cs.ds.iters != 0) {
		bIter = true;
	}

	for (i = 0; !bIter || i < cs.ds.iters; i++) {
		printf(LLUFMT" - processing\n", i);

		eh_ASSERT(cs.rs.block == NULL);
		eh_ASSERT(cs.rs.cooked == NULL);

		if (!getDirEntriesOf(&cs.data.hs, cs.data.dte,
					cs.data.szBase)) {
			if (iTooManyNonDirs++ > 60) {
				eh_fatal("%s(%d): Could not get any "
					"directory off of %s\n",
					__FILE__, __LINE__, cs.data.szBase);
			}

			sleep(1);
			continue;
		}

		iTooManyNonDirs = 0;

		if (cs.data.hs.uSumFiles == 0 && cs.ds.scan == scRead) {
			if (iTooManyNonFiles++ > 60) {
				eh_fatal("%s(%d): I am a reader and "
					"there are no files.\n",
					__FILE__, __LINE__);
			}

			if (!(iTooManyNonFiles % 4)) {
				printf("%s(%d): %ld times without a file "
					"to read.\n",
					__FILE__, __LINE__, iTooManyNonFiles);
			}

			sleep(1);
			continue;
		}

		iTooManyNonFiles = 0;

		cs.state.files.curr = cs.data.hs.uSumFiles;
		cs.state.dirs.curr = cs.data.hs.uSumDirs;

		/*
		 * Okay, determine our state.
		 */
		transitStates(&cs);

		/*
		 * Roll a die and take that action
		 */
		decideOnAction(&cs);

		destroyList(&cs.data.hs);
	}

	cleanupState(&cs);

	return (0);
}
