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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>

#include "elham.h"

/*
 * XXX: Make -block provide a range of blocks to dump.
 *      Could use cs.shuffled to store the entries.
 */

#define VERSION "$Id$"
#define USAGE                                                                \
  "Usage: datadump -file path [OPTION ...]\n"                                \
  "Version: "VERSION"\n"                                                     \
  "  -data PATH                   Base path for data files\n"                \
  "  -history PATH                Base path for history files\n"             \
  "  -meta PATH                   Base path for metafile files\n"            \
  "  -block N                     Block to dump\n"                           \
  "  -invalid                     Process blocks not in use\n"               \
  "  -verify                      Verify blocks not in use\n"                \
  "  -print                       Dump the blocks\n"                         \
  "\n"                                                                       \
  " The -file parameter must be supplied and it should be the base name\n"   \
  " with the base path and extension stripped off.\n"                        \
  "\n"                                                                       \
  " Example:\n"                                                              \
  " /x/eng/wafl/thomas/elham/tmp/data/1066.dlh/967.flh\n"                    \
  " becomes:\n"                                                              \
  " -file 1066.dlh/967\n"                                                    \
  "\n"                                                                       

static void 
processArgs (ControlStruct *pcs, int argc, char *argv[])
{
	strcpy(pcs->data.szBase, "./data");
	strcpy(pcs->meta.szBase, "./meta");
	strcpy(pcs->history.szBase, "./history");

	strcpy(pcs->data.szBase, "/t/blah/data");
	strcpy(pcs->meta.szBase, "/t/blah/meta");
	strcpy(pcs->history.szBase, "/t/blah/history");

	/*
	 * Fill in dsSupplied by what is supplied on the command line.
	 */
	for (NEXT_ARG(); argc > 0; NEXT_ARG()) {
		if (!strcmp(*argv, "-block")) {
			NEXT_ARG();
			NEED_ARG();
			pcs->hitThisBlock = atoll(*argv);
		} else if (!strcmp(*argv, "-file")) {
			NEXT_ARG();
			NEED_ARG();
			strncpy(pcs->szFile, *argv, EH_PATH_MAX);
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
		} else if (!strcmp(*argv, "-invalid")) {
			pcs->rs.dumpInvalid = true;
		} else if (!strcmp(*argv, "-verify")) {
			pcs->rs.verifyJunk = true;
		} else if (!strcmp(*argv, "-print")) {
			pcs->rs.print = true;
		} else if (!strcmp(*argv, "-version")) {
			printf(VERSION);
			exit(0);
		} else if (!strcmp(*argv, "-help")) {
			printf(USAGE);
			exit(0);
		} else {
			eh_fatal("Do not understand %s\n"USAGE, *argv);
		}
	}

	if (pcs->szFile[0] == '\0') {
		eh_fatal("Must provide a -file parameter\n"USAGE);
	}

	return;
}

int 
main(int argc, char *argv[])
{
	bool		b = false;
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

	snprintf(cs.data.szFile, EH_PATH_MAX,
		"%s/%s.flh", cs.data.szBase, cs.szFile);
	cs.data.bFullRange = true;
	cs.data.bByteLocks = false;
	cs.data.bWait = false;
	cs.data.bCreate = false;
	cs.data.bAppend = false;
	cs.data.access = amRead;
	cs.data.shareMode = smNone;

	if (eh_OpenFile(&cs.data) == INVALID_HANDLE_VALUE) {
		eh_fatal("%s(%d): Could not read %s\n",
			__FILE__, __LINE__, cs.data.szFile);
	}

	cs.meta.bFullRange = true;
	cs.meta.bByteLocks = false;
	cs.meta.bWait = false;
	cs.meta.bCreate = false;
	cs.meta.bAppend = false;
	cs.meta.access = amRead;
	cs.meta.shareMode = smNone;
	snprintf(cs.meta.szFile, EH_PATH_MAX,
		"%s/%s.flh", cs.meta.szBase, cs.szFile);

	if (eh_OpenFile(&cs.meta) == INVALID_HANDLE_VALUE) {
		eh_fatal("%s(%d): Could not read %s\n",
			__FILE__, __LINE__, cs.meta.szFile);
	}

	snprintf(cs.history.szFile, EH_PATH_MAX,
		"%s/%s.flh", cs.history.szBase, cs.szFile);

	readDFCB(cs.meta.fd, &cs.dfcb);

	if (cs.dfcb.signature != METAFILE_SIGNATURE) {
		eh_fatal("%s(%d): Metafile signature %#lx "
			"does not match %#lx\n",
			cs.dfcb.signature, METAFILE_SIGNATURE);
	}

	if (cs.dfcb.version != METAFILE_VERSION) {
		eh_fatal("%s(%d): Metafile version %#lx "
			"does not match %#lx\n",
			cs.dfcb.version, METAFILE_VERSION);
	}

	eh_ASSERT(!(cs.dfcb.fileSize % cs.dfcb.blockSize));

        /*
         * XXX: Do some more verifications!
         */

	printDFCB(cs.dfcb);

	cs.rs.blockSize = cs.dfcb.blockSize;

	/*
	 * Keep enough space to do some byte conversions on an extra 7 bytes.
	 */
	cs.rs.block = (char *)calloc(cs.rs.blockSize + 8, sizeof(char));
	if (!cs.rs.block) {
		eh_fatal("%s(%d): Could not allocate %d bytes\n",
			__FILE__, __LINE__, cs.rs.blockSize + 8);
	}

	cs.rs.cooked = (char *)calloc(cs.rs.blockSize + 8, sizeof(char));
	if (!cs.rs.cooked) {
		eh_fatal("%s(%d): Could not allocate %d bytes\n",
			__FILE__, __LINE__, cs.rs.blockSize + 8);
	}

	if (cs.hitThisBlock != -1) {
		if (cs.hitThisBlock >= cs.dfcb.blocks) {
			eh_fatal("-block "LLUFMT" must be less "
				"than blocks in the file "LLUFMT"\n",
				cs.hitThisBlock, cs.dfcb.blocks);
		}

		verifyTheRecord(&cs);
	} else {
		for (cs.hitThisBlock = 0;
			cs.hitThisBlock < cs.dfcb.blocks;
			cs.hitThisBlock++) {
			b = verifyTheRecord(&cs);
			if (b) {
				break;
			}
		}

		cs.hitThisBlock = -1;
	}

	free(cs.rs.cooked);
	cs.rs.cooked = (char *)NULL;

	free(cs.rs.block);
	cs.rs.block = (char *)NULL;

	eh_CloseFile(&cs.meta);
	eh_CloseFile(&cs.data);

	/*
	 * Write what we did to the history file.
	 */
	unwindHistory(&cs);

	return (0);
}
