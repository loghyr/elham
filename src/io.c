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
#include <ctype.h>

#include <errno.h>

#include "elham.h"

/*
 * Add padding to convert to network byte order when
 * the block size is not an even multiple of 8.  Yes,
 * it would be a hell of a lot easier to use multiples
 * of 4 and do uint4 storage, or even force it to be
 * a multiple of 8, _BUT_ we want traffic going across
 * the wire with an odd number (actually, just not a
 * multiple of 4) of bytes for testing, the god of
 * necessity, network traffic.
 */
char *
allocateBlockData (uint8 blockSize, char *szFile, int iLine)
{
	char *p;

	eh_ASSERT(blockSize != 0);
	eh_ASSERT(szFile);

	p = (char *)calloc(blockSize + sizeof(uint8), sizeof(char));
	if (!p) {
		eh_fatal("%s(%d): Could not allocate "LLUFMT" bytes\n",
			szFile, iLine, blockSize + sizeof(uint8));
	}

	return(p);
}

uint8 *
allocateShuffled (uint8 len, char *szFile, int iLine)
{
	uint8		i;
	uint8		i1;
	uint8		i2;
	uint8		t;
	uint8		end;

	uint8		*a;

	eh_ASSERT(len);
	eh_ASSERT(szFile);

	a = (uint8 *)calloc(len, sizeof(uint8));
	if (!a) {
		eh_fatal("%s(%d): Could not allocate "LLUFMT" bytes\n",
			szFile, iLine, len * sizeof(uint8));
	}

	for (i = 0; i < len; i++) {
		a[i] = i;
	}

	/*
	 * Shuffle the cards.
	 */
	for (i = 0, end = 2 * len; i < end; i++) {
		i1 = ROLL_UINT8(len, false);
		i2 = ROLL_UINT8(len, false);
		t = a[i1];
		a[i1] = a[i2];
		a[i2] = t;
	}

	return(a);
}

void
defaultStructCopy (DefaultsStruct *pdsDest, DefaultsStruct *pdsSource)
{
	pdsDest->minfsize = pdsSource->minfsize;
	pdsDest->maxfsize = pdsSource->maxfsize;
	pdsDest->minbsize = pdsSource->minbsize;
	pdsDest->maxbsize = pdsSource->maxbsize;
	pdsDest->blockSize = pdsSource->blockSize;
	pdsDest->seed = pdsSource->seed;
	pdsDest->flips = pdsSource->flips;
	pdsDest->width = pdsSource->width;
	pdsDest->depth = pdsSource->depth;
	pdsDest->acton = pdsSource->acton;
	pdsDest->files = pdsSource->files;
	pdsDest->iters = pdsSource->iters;
	pdsDest->scan = pdsSource->scan;
	pdsDest->wait = pdsSource->wait;
	pdsDest->rollBlocks = pdsSource->rollBlocks;
	pdsDest->actions.create = pdsSource->actions.create;
	pdsDest->actions.read = pdsSource->actions.read;
	pdsDest->actions.write = pdsSource->actions.write;
	pdsDest->actions.delete = pdsSource->actions.delete;
	pdsDest->actions.unlink = pdsSource->actions.unlink;
}

void
verifyControlStruct (ControlStruct *pcs)
{
	eh_ASSERT(pcs);

	/*
	 * First verify that the paths are valid.
	 */
	verifyBase(pcs->data.szBase);
	verifyBase(pcs->meta.szBase);
	verifyBase(pcs->history.szBase);

	/*
	 * Now verify that the block and file sizes are acceptable.
	 */
	if (!pcs->ds.rollBlocks) {
		if (pcs->ds.blockSize > MAXBLOCKSIZE) {
			eh_fatal("-blocksize "LLUFMT" can not be larger "
				"than the max block size of "LLUFMT"\n",
				pcs->ds.blockSize, MAXBLOCKSIZE);
		}

		if (pcs->ds.blockSize < MINBSIZE) {
			eh_fatal("-blocksize "LLUFMT" can not be less "
				"than the min block size of "LLUFMT"\n",
				pcs->ds.blockSize, MINBSIZE);
		}

		if (pcs->ds.blockSize > pcs->ds.minfsize) {
			eh_fatal("-blocksize "LLUFMT" can not be less "
				"than the -minfsize "LLUFMT"\n",
				pcs->ds.blockSize, pcs->ds.minfsize);
		}

		if (pcs->ds.blockSize > pcs->ds.maxfsize) {
			eh_fatal("-blocksize "LLUFMT" can not be larger "
				"than the -maxfsize "LLUFMT"\n",
				pcs->ds.blockSize, pcs->ds.maxfsize);
		}
	} else {
		if (pcs->ds.maxbsize > MAXBLOCKSIZE) {
			eh_fatal("-maxbsize "LLUFMT" can not be larger "
				"than the max block size of "LLUFMT"\n",
				pcs->ds.maxbsize, MAXBLOCKSIZE);
		}

		if (pcs->ds.minbsize < MINBSIZE) {
			eh_fatal("-minbsize "LLUFMT" can not be less "
				"than the min block size of "LLUFMT"\n",
				pcs->ds.minbsize, MINBSIZE);
		}

		if (pcs->ds.minbsize > pcs->ds.maxbsize) {
			eh_fatal("-minbsize "LLUFMT" can not be larger "
				"than the -maxbsize "LLUFMT"\n",
				pcs->ds.minbsize, pcs->ds.maxbsize);
		}

		if (pcs->ds.minbsize > pcs->ds.minfsize) {
			eh_fatal("-minbsize "LLUFMT" should not be greater "
				"than the -minfsize "LLUFMT"\n",
				pcs->ds.minbsize, pcs->ds.minfsize);
		}

		if (pcs->ds.minbsize > pcs->ds.maxfsize) {
			eh_fatal("-minbsize "LLUFMT" can not be larger "
				"than the -maxfsize "LLUFMT"\n",
				pcs->ds.minbsize, pcs->ds.maxfsize);
		}
	}

	if (pcs->ds.maxbsize > MAXFSIZE) {
		eh_fatal("-maxbsize "LLUFMT" can not be larger "
			"than the max file size of "LLUFMT"\n",
			pcs->ds.maxbsize, MAXFSIZE);
	}

	if (pcs->ds.minfsize < MINFSIZE) {
		eh_fatal("-minfsize "LLUFMT" can not be less "
			"than the min file size of "LLUFMT"\n",
			pcs->ds.minfsize, MINFSIZE);
	}

	if (pcs->ds.minfsize > pcs->ds.maxfsize) {
		eh_fatal("-minfsize "LLUFMT" can not be larger "
			"than the -maxfsize "LLUFMT"\n",
			pcs->ds.minfsize, pcs->ds.maxfsize);
	}
}

void
initControlStruct (ControlStruct *pcs)
{
	eh_ASSERT(pcs);

	/*
	 * Blank slate.
	 */
	memset(pcs, '\0', sizeof(ControlStruct));

	pcs->dsLock.minfsize = MINFSIZE;
	pcs->dsLock.maxfsize = MAXFSIZE;
	pcs->dsLock.minbsize = MINBSIZE;
	pcs->dsLock.maxbsize = MAXBLOCKSIZE;
	pcs->dsLock.blockSize = 128;
	pcs->dsLock.rollBlocks = true;
	pcs->dsLock.seed = 0x10c45a11;
	pcs->dsLock.flips = 0;
	pcs->dsLock.width = 2;
	pcs->dsLock.depth = 2;
	pcs->dsLock.acton = 10;
	pcs->dsLock.files = 10;
	pcs->dsLock.iters = 0;
	pcs->dsLock.scan = scRandom;
	pcs->dsLock.wait = weRandom;
	pcs->dsLock.actions.create = 20;
	pcs->dsLock.actions.read = 20;
	pcs->dsLock.actions.write = 20;
	pcs->dsLock.actions.delete = 20;
	pcs->dsLock.actions.unlink = 20;

	pcs->dsHammer.minfsize = MINFSIZE;
	pcs->dsHammer.maxfsize = MAXFSIZE;
	pcs->dsHammer.minbsize = MINBSIZE;
	pcs->dsHammer.maxbsize = MAXBLOCKSIZE;
	pcs->dsHammer.blockSize = MAXBLOCKSIZE / 1024;
	pcs->dsHammer.rollBlocks = true;
	pcs->dsHammer.seed = 0x55097734;
	pcs->dsHammer.flips = 0;
	pcs->dsHammer.width = 10240;
	pcs->dsHammer.depth = 6;
	pcs->dsHammer.acton = 100;
	pcs->dsHammer.files = 100;
	pcs->dsHammer.iters = 0;
	pcs->dsHammer.scan = scRandom;
	pcs->dsHammer.wait = weRandom;
	pcs->dsHammer.actions.create = 10;
	pcs->dsHammer.actions.read = 30;
	pcs->dsHammer.actions.write = 30;
	pcs->dsHammer.actions.delete = 20;
	pcs->dsHammer.actions.unlink = 10;

	pcs->dsDefault.minfsize = MINFSIZE;
	pcs->dsDefault.maxfsize = MAXFSIZE;
	pcs->dsDefault.minbsize = MINBSIZE;
	pcs->dsDefault.maxbsize = MAXBLOCKSIZE;
	pcs->dsDefault.blockSize = MAXBLOCKSIZE / 2048;
	pcs->dsDefault.rollBlocks = true;
	pcs->dsDefault.seed = 0xdefacade;
	pcs->dsDefault.flips = 0;
	pcs->dsDefault.width = 10240;
	pcs->dsDefault.depth = 4;
	pcs->dsDefault.acton = 50;
	pcs->dsDefault.files = 1000;
	pcs->dsDefault.iters = 0;
	pcs->dsDefault.scan = scRandom;
	pcs->dsDefault.wait = weRandom;
	pcs->dsDefault.actions.create = 20;
	pcs->dsDefault.actions.read = 20;
	pcs->dsDefault.actions.write = 20;
	pcs->dsDefault.actions.delete = 20;
	pcs->dsDefault.actions.unlink = 20;

	defaultStructCopy(&pcs->dsSupplied, &pcs->dsDefault);

	pcs->defaultTo = deDefault;

	pcs->data.dte = dteData;
	pcs->meta.dte = dteMeta;
	pcs->history.dte = dteHistory;

	pcs->data.fd = INVALID_HANDLE_VALUE;
	pcs->meta.fd = INVALID_HANDLE_VALUE;
	pcs->history.fd = INVALID_HANDLE_VALUE;

	pcs->data.iOpenFailures = 0;
	pcs->meta.iOpenFailures = 0;
	pcs->history.iOpenFailures = 0;

	pcs->hitThisBlock = -1;
}

void
printDFCB (DataFileControlBlock dfcb)
{
	printf("=== dfcb ===\n");
	printf("Signature:        %#lx\n", dfcb.signature);
	printf("Version:          %ld\n", dfcb.version);
	printf("Block Size:       "LLUFMT"\n", dfcb.blockSize);
	printf("File Size:        "LLUFMT"\n", dfcb.fileSize);
	printf("Number of Blocks: "LLUFMT"\n", dfcb.blocks);
	printf("Seed:             %#lx\n", dfcb.seed);
	printf("Flips:            "LLUFMT"\n", dfcb.flips);
	printf("Corrupt:          %s\n",
		dfcb.corrupt ? "true" : "false");
	printf("=== dfcb ===\n");
}

void
printBlock (BlockStruct bs, uint8 number, uint8 max)
{
	printf("=== Block "LLUFMT" of "LLUFMT" ===\n",
			number, max);
	printf("Offset:           "LLXFMT"\n", bs.offset);
	printf("Size:             "LLXFMT"\n", bs.blockSize);
	printf("Seed:             %#lx\n", bs.seed);
	printf("Flips:            "LLUFMT"\n", bs.flips);
	printf("Pattern:          "LLXFMT"\n", bs.pattern);
	printf("In Use:           %s\n", useStringOf((UseEnum)bs.inUse));
	printf("Corrupt:          %s\n",
		bs.corrupt ? "true" : "false");
	printf("Create Pid:       %ld\n", bs.createPid);
	printf("Create Time:      (%ld, %ld)\n",
		bs.createTime.tv_sec, bs.createTime.tv_usec);
	printf("Modify Pid:       %ld\n", bs.modifyPid);
	printf("Modify Time:      (%ld, %ld)\n",
		bs.modifyTime.tv_sec, bs.modifyTime.tv_usec);
	printf("Read Pid:         %ld\n", bs.readPid);
	printf("Read Time:        (%ld, %ld)\n",
		bs.readTime.tv_sec, bs.readTime.tv_usec);
	printf("Access Pid:       %ld\n", bs.accessPid);
	printf("Access Time:      (%ld, %ld)\n",
		bs.accessTime.tv_sec, bs.accessTime.tv_usec);
	printf("=== Block "LLUFMT" of "LLUFMT" ===\n", number, max);
}

void
printHistory (HistoryStruct hs, uint8 number)
{
	char	*szAction = (char *)NULL;

	szAction = actionStringOf((ActionEnum)hs.action);
	eh_ASSERT(szAction);

	printf("=== History "LLUFMT" ===\n", number);
	printf("Action:           %s\n", szAction);
	printf("Time:             (%ld, %ld)\n",
			hs.time.tv_sec, hs.time.tv_usec);
	printf("Block:            "LLUFMT"\n", hs.block);
	printf("Block Size:       "LLUFMT"\n", hs.blockSize);
	printf("Pattern:          "LLXFMT"\n", hs.pattern);
	printf("Pid:              %ld\n", hs.pid);
	printf("Mid:              %ld\n", hs.mid);
	printf("Seed:             %#lx\n", hs.seed);
	printf("Flips:            "LLUFMT"\n", hs.flips);
	printf("Corrupt:          %s\n", hs.corrupt ? "true" : "false");
	printf("In Use:           %s\n", useStringOf((UseEnum)hs.inUse));
	printf("=== History "LLUFMT" ===\n", number);
}

int
readHistory (Filedesc_t fd, HistoryStruct *pout)
{
	HistoryStruct	in;
	int		iRC;

	iRC = eh_Read(fd, &in, sizeof(HistoryStruct));

	NTOHTV(&pout->time, &in.time);
	pout->seed = ntohl(in.seed);
	pout->action = in.action;
	pout->corrupt = in.corrupt;
	pout->inUse = in.inUse;
	NTOHU8(&pout->block, &in.block);
	NTOHU8(&pout->blockSize, &in.blockSize);
	NTOHU8(&pout->flips, &in.flips);
	NTOHU8(&pout->pattern, &in.pattern);
	pout->pid = ntohl(in.pid);
	pout->mid = ntohl(in.mid);

	return(iRC);
}

int
writeHistory (Filedesc_t fd, HistoryStruct *pin)
{
	HistoryStruct	out;
	int		iRC;

	HTONTV(&out.time, &pin->time);
	out.seed = htonl(pin->seed);
	out.action = pin->action;
	out.corrupt = pin->corrupt;
	out.inUse = pin->inUse;
	HTONU8(&out.block, &pin->block);
	HTONU8(&out.blockSize, &pin->blockSize);
	HTONU8(&out.flips, &pin->flips);
	HTONU8(&out.pattern, &pin->pattern);
	out.pid = htonl(pin->pid);
	out.mid = htonl(pin->mid);

	iRC = eh_Write(fd, &out, sizeof(HistoryStruct));

	return(iRC);
}

void
printRecord (RecordStruct rs, uint8 number)
{
	uint8	i;
	uint8	j;
	uint8	displayed = 0;
	int	c;
	char	*pc = (char *)NULL;
	char	asc[17];
	char	*pasc = (char *)NULL;
	bool	bInhibit = false;
	Offset_t	pos = rs.pos - (rs.pos % 16);
	int	slide;

	eh_ASSERT(rs.block);
	eh_ASSERT(rs.cooked);

       	byteSwapData(rs.cooked, rs.block, rs.blockSize);
	pc = rs.cooked;

	printf("=== Data "LLUFMT" ===\n", number);

	for (i = 0, displayed = 0; displayed < rs.blockSize; i += 16) {
		memset(asc, ' ', 16);
		asc[16] = '\0';
		pasc = asc;

		printf(LL8XFMT"  ", pos + i);

		if (i == 0) {
			slide = rs.pos % 16;

			if (!slide) {
				j = i;
				bInhibit = false;
			} else {
				for (j = 0;
					j < slide && j < (i + 8);
					j++, pasc++) {
					printf("   ");
				}

				if (j < slide) {
					printf("  ");
					bInhibit = true;
				}

				for ( ;
					j < slide && j < (i + 16);
					j++, pasc++) {
					printf("   ");
				}
			}
		} else {
			j = i;
			bInhibit = false;
		}

		for ( ;
			displayed < rs.blockSize && j < (i + 8);
			j++, displayed++) {
			c = *pc++ & 0xFF;
			*pasc++ = isprint((int)c) ? c : '.';
			printf("%02X ", c);
		}

		for ( ; j < (i + 8); j++) {
			printf("   ");
		}

		if (!bInhibit) {
			if (displayed < rs.blockSize) {
				printf("- ");
			} else {
				printf("  ");
			}
		}

		for ( ;
			displayed < rs.blockSize && j < (i + 16);
			j++, displayed++) {
			c = *pc++ & 0xFF;
			*pasc++ = isprint((int)c) ? c : '.';
			printf("%02X ", c);
		}

		for ( ; j < (i + 16); j++) {
			printf("   ");
		}

		printf("%s\n", asc);
	}

	printf("=== Data "LLUFMT" ===\n", number);
}

uint8
readRecord (Filedesc_t fd, RecordStruct *prs)
{
	int		iRC;
	Offset_t	oRet;

	eh_ASSERT(prs);

	oRet = lseek(fd, prs->pos, SEEK_SET);
	if (oRet == -1) {
		fprintf(stderr, "Hit unexpected error in lseek\n");
		syserror();
	}

	iRC = eh_Read(fd, prs->cooked, prs->blockSize);
	byteSwapData(prs->block, prs->cooked, prs->blockSize);

	return(iRC);
}

uint8
writeRecord (Filedesc_t fd, RecordStruct *prs)
{
	int		iRC;
	Offset_t	oRet;

	eh_ASSERT(prs);

	byteSwapData(prs->cooked, prs->block, prs->blockSize);

	oRet = lseek(fd, prs->pos, SEEK_SET);
	if (oRet == -1) {
		fprintf(stderr, "Hit unexpected error in lseek\n");
		syserror();
	}
	printf("Writing "LLUFMT" bytes to "LLUFMT"\n",
		prs->blockSize, prs->pos);

	iRC = eh_Write(fd, prs->cooked, prs->blockSize);

	return(iRC);
}

int
readDFCB (Filedesc_t fd, DataFileControlBlock *pout)
{
	DataFileControlBlock	in;
	int		iRC;

	eh_ASSERT(pout);

	iRC = eh_Read(fd, &in, sizeof(DataFileControlBlock));

	pout->signature = ntohl(in.signature);
	pout->version = ntohl(in.version);
	NTOHU8(&pout->fileSize, &in.fileSize);
	NTOHU8(&pout->blocks, &in.blocks);
	NTOHU8(&pout->blockSize, &in.blockSize);
	pout->seed = ntohl(in.seed);
	NTOHU8(&pout->flips, &in.flips);
	pout->corrupt = in.corrupt;

	return(iRC);
}

int
writeDFCB (Filedesc_t fd, DataFileControlBlock *pin)
{
	DataFileControlBlock	out;
	int		iRC;

	eh_ASSERT(pin);

	out.signature = htonl(pin->signature);
	out.version = htonl(pin->version);
	HTONU8(&out.fileSize, &pin->fileSize);
	HTONU8(&out.blocks, &pin->blocks);
	HTONU8(&out.blockSize, &pin->blockSize);
	out.seed = htonl(pin->seed);
	HTONU8(&out.flips, &pin->flips);
	out.corrupt = pin->corrupt;

	iRC = eh_Write(fd, &out, sizeof(DataFileControlBlock));

	return(iRC);
}

int
readBlock (Filedesc_t fd, BlockStruct *pout)
{
	BlockStruct	in;
	int		iRC;

	eh_ASSERT(pout);
	iRC = eh_Read(fd, &in, sizeof(BlockStruct));

	NTOHU8(&pout->offset, &in.offset);
	NTOHU8(&pout->blockSize, &in.blockSize);
	NTOHU8(&pout->flips, &in.flips);
	NTOHU8(&pout->pattern, &in.pattern);
	pout->seed = ntohl(in.seed);
	pout->createPid = ntohl(in.createPid);
	pout->modifyPid = ntohl(in.modifyPid);
	pout->readPid = ntohl(in.readPid);
	pout->accessPid = ntohl(in.accessPid);
	NTOHTV(&pout->createTime, &in.createTime);
	NTOHTV(&pout->modifyTime, &in.modifyTime);
	NTOHTV(&pout->readTime, &in.readTime);
	NTOHTV(&pout->accessTime, &in.accessTime);
	pout->inUse = in.inUse;
	pout->corrupt = in.corrupt;

	return(iRC);
}

int
writeBlock (Filedesc_t fd, BlockStruct *pin)
{
	BlockStruct	out;
	int		iRC;

	eh_ASSERT(pin);

	HTONU8(&out.offset, &pin->offset);
	HTONU8(&out.blockSize, &pin->blockSize);
	HTONU8(&out.flips, &pin->flips);
	HTONU8(&out.pattern, &pin->pattern);
	out.seed = htonl(pin->seed);
	out.createPid = htonl(pin->createPid);
	out.modifyPid = htonl(pin->modifyPid);
	out.readPid = htonl(pin->readPid);
	out.accessPid = htonl(pin->accessPid);
	HTONTV(&out.createTime, &pin->createTime);
	HTONTV(&out.modifyTime, &pin->modifyTime);
	HTONTV(&out.readTime, &pin->readTime);
	HTONTV(&out.accessTime, &pin->accessTime);
	out.inUse = pin->inUse;
	out.corrupt = pin->corrupt;

	iRC = eh_Write(fd, &out, sizeof(BlockStruct));

	return(iRC);
}
