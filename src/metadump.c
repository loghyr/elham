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

## @summary Scan metafiles for ElHam.
## @pod here
## @author thomas@netapp.com

=head1 Preamble

Read in a metafile and print out its contents.

=cut
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>

#include "elham.h"

#define VERSION "$Id$"
#define USAGE                                                                \
  "Usage: metadump path\n"                                                   \
  "Version: "VERSION"\n"                                                    

int 
main(int argc, char *argv[])
{
	int			i;
	BlockStruct		bs;
	DataFileControlBlock	dfcb;
	Pid_t			pid;
	FileStruct		meta;

	uint8			rw;

	if (argc != 2) {
		eh_fatal(USAGE);
	}

#if defined (unix)
	pid = getpid();
#else
	pid = _getpid();
#endif

	printf("My pid is %ld\n", pid);
	
	meta.bFullRange = true;
	meta.bByteLocks = false;
	meta.bWait = false;
	meta.bCreate = false;
	meta.bAppend = false;
	meta.access = amRead;
	meta.shareMode = smNone;
	strncpy(meta.szFile, argv[1], EH_PATH_MAX);

	if (eh_OpenFile(&meta) == -1) {
		eh_fatal("%s(%d): Could not open for reading %s\n",
				__FILE__, __LINE__, meta.szFile);
	}

	rw = readDFCB(meta.fd, &dfcb);
	if (rw != sizeof(DataFileControlBlock)) {
		eh_fatal("%s(%d): Could not read the data file"
			" control block for %s\n",
			__FILE__, __LINE__, meta.szFile);
	}
	printDFCB(dfcb);

	for (i = 0; i < dfcb.blocks; i++) {
		rw = readBlock(meta.fd, &bs);
		if (rw != sizeof(BlockStruct)) {
			eh_fatal("%s(%d): Could not read the %d'th"
				" block for %s\n",
				__FILE__, __LINE__, i, meta.szFile);
		}
		printBlock(bs, i, dfcb.blocks);
	}

	eh_CloseFile(&meta);

	return (0);
}
