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
 * Routines for Endianess
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <errno.h>

#include "elham.h"

/*
 * We treat the data block as if it were written as
 * an 8 byte entity.  If the block size is not a 
 * multiple of 8, we handle the last 8 bytes accordingly.
 * size   write pattern
 * 8      uint8
 * 7      uint4 uint2 uint1
 * 6      uint4 uint2
 * 5      uint4 uint1
 * 4      uint4
 * 3      uint2 uint1
 * 2      uint2
 * 1      uint1
 */
void
byteSwapData (char *pout, char *pin, uint8 blockSize)
{
	uint8		i;
#if !defined WORDS_BIGENDIAN
	uint8		effBlockSize;
#endif

	eh_ASSERT(pin);
	eh_ASSERT(pout);

#if defined WORDS_BIGENDIAN
	for (i = 0; i < blockSize; i++) {
		pout[i] = pin[i];
	}
#else
	effBlockSize = blockSize - (blockSize % sizeof(uint8));
	if (blockSize % sizeof(uint8)) {
		effBlockSize += sizeof(uint8);
	}

	for (i = 0; i < effBlockSize; i += sizeof(uint8)) {
		pout[i + 3] = pin[i + 4];
		pout[i + 2] = pin[i + 5];
		pout[i + 1] = pin[i + 6];
		pout[i + 0] = pin[i + 7];
		pout[i + 7] = pin[i + 0];
		pout[i + 6] = pin[i + 1];
		pout[i + 5] = pin[i + 2];
		pout[i + 4] = pin[i + 3];
	}
#endif
}
