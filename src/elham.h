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
 * This header defines the data structures necessary to test
 * lock manager functionality.
 */

#ifndef _ELHAM_H_
#define _ELHAM_H_

#include <config.h>

#include <stdio.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif


#if HAVE_STDBOOL_H
# include <stdbool.h>
#else
# if ! HAVE__BOOL
# ifdef __cplusplus
typedef bool _Bool;
# else
typedef unsigned char _Bool;
# endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif 

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if defined (unix)

#include <fcntl.h>

#include <sys/uio.h>

#include <netinet/in.h>

typedef int     Filedesc_t;

#if defined(__alpha__) || defined(__alpha)
#define LONG_IS_EIGHTBYTES    1
#else
#include <inttypes.h>
#endif

#define MAXCLIENTNAME          256
#define MAXCWDLEN              MAXPATHLEN

#if LONG_IS_EIGHTBYTES

typedef long int8;
typedef unsigned long uint8;
typedef unsigned int uint4;

#define atoll          atol
#define LLFMT          "%ld"
#define LLUFMT         "%lu"
#define LLXFMT         "%#lx"
#define LL8XFMT        "%#8.8lx"

#elif __GNUC__

#if defined (int64_t)
typedef int64_t int8;
#else
typedef long long int8;
#endif

#if defined (uint64_t)
typedef uint64_t uint8;
#elif defined (u_int64_t)
typedef u_int64_t uint8;
#else
typedef unsigned long long uint8;
#endif

#if defined (uint32_t)
typedef uint32_t uint4;
#elif defined (u_int32_t)
typedef u_int32_t uint4;
#else
typedef unsigned long uint4;
#endif

#if defined (__FreeBSD__)
#define atoll(nptr)     strtoq((nptr), NULL, 10)
#else
/* assumes the presence of atoll() */
#endif

#define LLFMT          "%lld"
#define LLUFMT         "%llu"
#define LLXFMT         "%#llx"
#define LL8XFMT        "%#8.8llx"

#endif /* LONG_IS_EIGHTBYTES */

#if !defined (_FILE_OFFSET_BITS)
#define _FILE_OFFSET_BITS 64
#endif

#define STRICMP(A,B) strcasecmp((A),(B))
#define STRNICMP(A,B,C) strncasecmp((A),(B),(C))

#define FILE_MODE       (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define INVALID_HANDLE_VALUE -1

#define DIR_SEP '/'
#define DIR_SEP_STR "/"

/*
 * Need a better way of getting to 64 bit values.
 *
 * This handles Solaris (2.8 and above).
 * Linux seems to define F_SETLK64 == F_SETLK,
 * so we end up being okay.
 */
#if defined(__sparc)

#define eh_F_SETLK	F_SETLK64
#define eh_F_SETLKW	F_SETLKW64
#define eh_F_GETLK	F_GETLK64
#define eh_F_FREESP	F_FREESP64

#define SOLARIS_LARGE_LOCKS 1

#else

#define eh_F_SETLK	F_SETLK
#define eh_F_SETLKW	F_SETLKW
#define eh_F_GETLK	F_GETLK
#define eh_F_FREESP	F_FREESP
#endif

typedef uint8   Offset_t;
typedef uint8   Pid_t;

#define READ_LOCK(fd, offset, whence, len) \
                        lock_reg(fd, eh_F_SETLK, F_RDLCK, offset, whence, len)
#define READW_LOCK(fd, offset, whence, len) \
                        lock_reg(fd, eh_F_SETLKW, F_RDLCK, offset, whence, len)
#define WRITE_LOCK(fd, offset, whence, len) \
                        lock_reg(fd, eh_F_SETLK, F_WRLCK, offset, whence, len)
#define WRITEW_LOCK(fd, offset, whence, len) \
                        lock_reg(fd, eh_F_SETLKW, F_WRLCK, offset, whence, len)
#define UN_LOCK(fd, offset, whence, len) \
                        lock_reg(fd, eh_F_SETLK, F_UNLCK, offset, whence, len)

#define IS_READ_LOCKABLE(fd, offset, whence, len) \
                        !lock_test(fd, F_RDLCK, offset, whence, len)
#define IS_WRITE_LOCKABLE(fd, offset, whence, len) \
                        !lock_test(fd, F_WRLCK, offset, whence, len)

#define READ_LOCK_IF_ABLE(fd, offset, whence, len, iRC) \
	if (IS_READ_LOCKABLE((fd), (offset), (whence), (len))) { \
		(iRC) = READ_LOCK((fd), (offset), (whence), (len));      \
	} else { \
		(iRC) = -1; \
	}
#define WRITE_LOCK_IF_ABLE(fd, offset, whence, len, iRC) \
	if (IS_WRITE_LOCKABLE((fd), (offset), (whence), (len))) { \
		(iRC) = WRITE_LOCK((fd), (offset), (whence), (len));      \
	} else { \
		(iRC) = -1; \
	}

extern int lock_reg (int, int, int, Offset_t, int, off_t);
extern Pid_t lock_test (int, int, Offset_t, int, off_t);

#if !defined (O_BINARY)
#define O_BINARY 0
#endif

/*
 * Windows
 */
#else

#include <windows.h>
#include <conio.h>

typedef HANDLE  Filedesc_t;

#define STRICMP(A,B) _stricmp((A),(B))
#define STRNICMP(A,B) _strincmp((A),(B),(C))

#define sleep(A) SleepEx((DWORD)((A) * 1000), (bool)0);

extern int _getpid(void);

#include <direct.h>
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define mkdir(path, mode) mkdir(path)

#define MAXCLIENTNAME 256
#define MAXCWDLEN _MAX_PATH

#define DIR_SEP '\\'
#define DIR_SEP_STR "\\"

typedef __int64 int8;
typedef unsigned __int64 uint8;
typedef unsigned __int32 uint4;

typedef unsigned __int64 Offset_t;
typedef unsigned __int64 Pid_t;

#define atoll          _atoi64
#define LLFMT          "%I64d"
#define LLUFMT         "%I64u"
#define LLXFMT         "%#I64x"
#define LL8XFMT        "%#8.8I64x"

/*
 * Windows defines
 */
#endif

#if defined WORDS_BIGENDIAN
#define ENDY_QUAD_HIGH 0
#define ENDY_QUAD_LOW  1
#else
#define ENDY_QUAD_HIGH 1
#define ENDY_QUAD_LOW  0
#endif

#define NTOHTV(to, from) { \
	(to)->tv_sec = ntohl((from)->tv_sec); \
	(to)->tv_usec = ntohl((from)->tv_usec); \
	}

#define HTONTV(to, from) { \
	(to)->tv_sec = htonl((from)->tv_sec); \
	(to)->tv_usec = htonl((from)->tv_usec); \
	}

#define NTOHU8(to, from) { \
	((uint4 *)(to))[ENDY_QUAD_HIGH] = ntohl(((uint4 *)(from))[0]); \
	((uint4 *)(to))[ENDY_QUAD_LOW] = ntohl(((uint4 *)(from))[1]); \
	}

#define HTONU8(to, from) { \
	((uint4 *)(to))[0] = htonl(((uint4 *)(from))[ENDY_QUAD_HIGH]); \
	((uint4 *)(to))[1] = htonl(((uint4 *)(from))[ENDY_QUAD_LOW]); \
	}

#define KILOBYTE ((int8)1024)
#define MEGABYTE ((int8)1024 * KILOBYTE)
#define GIGABYTE ((int8)1024 * MEGABYTE)
#define TERABYTE ((int8)1024 * GIGABYTE)

#define MINFSIZE        4
#define MINBSIZE        MINFSIZE
#if LONG_IS_EIGHTBYTES
#define MAXFSIZE       0x7fffffffffffffffL
#elif __GNUC__
#define MAXFSIZE       0x7fffffffffffffffLL
#else
#define MAXFSIZE       (int8)0x7fffffffffffffff
#endif

#define MAXBLOCKSIZE    (64 * 1024 * 1024)
#define KPS(bytes,secs) (((bytes) / 1024.0) / (secs))

#define eh_DEBUG

#define eh_ASSERT(A) \
	if (!(A)) { \
		eh_fatal("%s(%d): Assertion failed \"%s\"\n", \
			__FILE__, __LINE__, #A); \
	}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef EH_PATH_MAX
#define EH_PATH_MAX        1024	/* max # of characters in a pathname */
#endif

#define FATAL_USAGE(str) eh_fatal("Do not understand %s\n"USAGE, (str));
#define NEXT_ARG()      (argc--, argv++)
#define NEED_ARG() { \
	if (argc < 1) { \
		eh_fatal("%s needs an arguement\n"USAGE, *(argv - 1)); \
	} \
}

#define METAFILE_SIGNATURE 0x540177ed
#define METAFILE_VERSION 0x1

#define SOILED_BLOCK_PATTERN 0xbeadc4e5031e1d49LL

#define BASE_DATA_PATH "/t/data"
#define BASE_META_PATH "/t/meta"
#define BASE_HISTORY_PATH "/t/history"

#define DOCFMT "%13.13s"

typedef enum AccessEnum {
	amWrite = 0,
	amRead,
	amRW,
	amMAX
} AccessEnum;

typedef enum ActionEnum {
	aeCreate = 0,
	aeRead,
	aeWrite,
	aeDelete,
	aeUnlink,
	aeMAX
} ActionEnum;

typedef enum DefaultsEnum {
	deLock = 0,
	deHammer,
	deDefault,
	deMAX
} DefaultsEnum;

typedef enum DirTypeEnum {
	dteData = 0,
	dteMeta,
	dteParity,
	dteSums,
	dteHistory,
	dteMAX
} DirTypeEnum;

typedef enum FindDirEnum {
	fdFile = 0,
	fdDir,
	fdEither,
	fdMAX
} FindDirEnum;

typedef enum ScanEnum {
	scRead = 0,
	scWrite,
	scRandom,
	scMAX
} ScanEnum;

typedef enum ShareModeEnum {
	smAll = 0,
	smRead,
	smWrite,
	smNone,
	smMAX
} ShareModeEnum;

typedef enum StateEnum {
	stEmpty = 0,
	stSoftEmpty,
	stHardEmpty,
	stNormal,
	stSoftFull,
	stHardFull,
	stFull,
	stMAX
} StateEnum;

typedef enum UseEnum {
	usVirgin = 0,
	usActive,
	usSoiled,
	usMAX
} UseEnum;

typedef enum WaitEnum {
	weAlways = 0,
	weNever,
	weRandom,
	weMAX
} WaitEnum;

/*
 * In core.
 */
typedef struct ActionWeightsStruct {
	uint4	create;
	uint4	read;
	uint4	write;
	uint4	delete;
	uint4	unlink;
} ActionWeightsStruct;

/*
 * In core.
 */
typedef struct DirEntryStruct {
	struct DirEntryStruct	*pdsNext;
	struct DirEntryStruct	*pdsPrev;
	struct HeadStruct	*phs;
	bool		bDir;
	bool		bStream;
	bool		bScanned;
	uint8		uId;
	char		szName[EH_PATH_MAX];
} DirEntryStruct;

/*
 * In core.
 */
typedef struct HeadStruct {
	uint8		uId;
	uint8		uDirs;
	uint8		uFiles;
	uint8		uSumDirs;
	uint8		uSumFiles;
	DirEntryStruct	*pdsHead;
	DirEntryStruct	*pdsTail;
	DirEntryStruct	*pdsCurr;
} HeadStruct;

/*
 * On disk, meta.
 */
typedef struct DataFileControlBlock {
	uint4		signature;
	uint4		version;
	uint8		blocks;
	uint8		blockSize;
	uint8		flips;
	uint8		fileSize;
	uint4		seed;
	bool		corrupt;
	unsigned char	uc5;
	unsigned char	uc6;
	unsigned char	uc7;
} DataFileControlBlock;

/*
 * On disk, meta.
 */
typedef struct BlockStruct {
	Offset_t	offset;
	Offset_t	blockSize;
	uint8		flips;
	uint8		pattern;
	struct timeval	createTime;
	struct timeval	modifyTime;
	struct timeval	accessTime;
	struct timeval	readTime;
	Pid_t		createPid;
	Pid_t		modifyPid;
	Pid_t		accessPid;
	Pid_t		readPid;
	uint4		seed;
	unsigned char	inUse;  /* Don't waste 7 bytes! */
	bool		corrupt;
	unsigned char	uc6;
	unsigned char	uc7;
} BlockStruct;

/*
 * In core.
 */
typedef struct FileStruct {
	Filedesc_t	fd;
	bool		bByteLocks;
	bool		bFullRange;
	bool		bWait;
	bool		bFullLocked;
	bool		bLocked;
	bool		bAppend;
	bool		bCreate;
	HeadStruct	hs;
	AccessEnum	access;
	ShareModeEnum	shareMode;
	ShareModeEnum	byteShare;
	DirTypeEnum	dte;
	Offset_t	oBaseStart;
	Offset_t	oBaseLen;
	Offset_t	oStart;
	Offset_t	oRange;
	uint4		iOpenFailures;
#if !defined (unix)
	OVERLAPPED	offset;
#endif
	char		szBase[EH_PATH_MAX];
	char		szFile[EH_PATH_MAX];
} FileStruct;

/*
 * In core.
 */
typedef struct DefaultsStruct {
	uint8		minfsize;
	uint8		maxfsize;
	uint8		minbsize;
	uint8		maxbsize;
	uint8		blockSize;
	uint8		flips;
	uint8		width;
	uint8		depth;
	uint8		acton;
	uint8		files;
	uint8		iters;
	uint4		seed;
	ScanEnum	scan;
	WaitEnum	wait;
	bool		rollBlocks;
	ActionWeightsStruct	actions;
} DefaultsStruct;

/*
 * On disk, history.
 */
typedef struct HistoryStruct {
	struct timeval	time;
	uint4		seed;
	unsigned char	action;  /* Should be ActionEnum, */
				 /* but that wastes 7 bytes */
	bool		corrupt;
	unsigned char	inUse;   /* Don't waste 7 bytes! */
	unsigned char	uc7;
	uint8		flips;
	uint8		pattern;
	uint8		block;
	uint8		blockSize;
	Pid_t		pid;
	uint4		mid;
} HistoryStruct;		

/*
 * In core.
 *
 * We don't want the on disk structure to waste space on
 * a pointer.
 */
typedef struct HistoryQueueStruct {
	HistoryStruct	hs;
	struct HistoryQueueStruct	*next;
	struct HistoryQueueStruct	*prev;
} HistoryQueueStruct;

/*
 * On disk, history.
 */
typedef struct MachineNameStruct {
	uint4		mid;
	uint4		crap;
	char		szName[MAXCLIENTNAME];
} MachineNameStruct;

/*
 * In core.
 */
typedef struct RecordStruct {
	char		*cooked;
	char		*block;
	uint8		blockSize;
	Offset_t	pos;
	bool		dumpInvalid;
	bool		print;
	bool		verifyJunk;
} RecordStruct;

/*
 * In core.
 */
typedef struct LimitStruct {
	uint8		limit;
	uint8		soft;
	uint8		hard;
} LimitStruct;

/*
 * In core.
 */
typedef struct TriggerStruct {
	uint8		curr;
	uint4		iGrades;
	uint8		*grades;
	LimitStruct	min;
	LimitStruct	max;
	StateEnum	*state;
	bool		bRotate;
	uint4		iMemory;
	char		name[16];
} TriggerStruct;

typedef struct NamelessStruct {
	TriggerStruct	files;
	TriggerStruct	blocks;
} NamelessStruct;

/*
 * In core.
 */
typedef struct StateStruct {
	TriggerStruct	files;
	TriggerStruct	dirs;
	NamelessStruct	meta;
	NamelessStruct	data;
	NamelessStruct	history;
} StateStruct;

/*
 * In core.
 */
typedef struct PathStackStruct {
	char	szName[EH_PATH_MAX];
	uint8	depth;
	struct PathStackStruct	*next;
} PathStackStruct;

/*
 * In core.
 */
typedef struct ControlStruct {
	FileStruct	data;
	FileStruct	meta;
	FileStruct	history;

	/*
	 * While processing the command line arguments, the user can select
	 * canned defaults for:
	 * Set     - command line option
	 * default - by default
	 * lock    - -lock
	 * hammer  - -hammer
	 *
	 * These are mutually exclusive and will form the base for the values
	 * passed in on the command line.  Since we are not ordering the options
	 * on the command line, we collect command line values in dsSupplied.
	 *
	 * After all argument processing is done:
	 * if defaultTo == deDefault
	 *    copy dsSupplied to ds
	 *    (dsSupplied defaults to dsDefault)
	 * else if defaultTo == deLock
	 *    copy dsLock to ds
	 *    foreach value in dsSupplied not matching the value in dsDefault
	 *       copy dsSupplied.value to ds.value
	 *    end
	 * else if defaultTo == deHammer
	 *    copy dsLock to ds
	 *    foreach value in dsSupplied not matching the value in dsDefault
	 *       copy dsSupplied.value to ds.value
	 *    end
	 * end
	 *
	 * (Ordering the options might have made a better algorithm.;)
	 */
	DefaultsStruct	dsLock;
	DefaultsStruct	dsHammer;
	DefaultsStruct	dsDefault;
	DefaultsStruct	dsSupplied;
	DefaultsStruct	ds;
	DefaultsEnum	defaultTo;

	ActionWeightsStruct	actions;

	ActionEnum		takeAction;

	StateStruct		state;

	Pid_t			pid;

	DataFileControlBlock	dfcb;
	BlockStruct		bs;
#if 0
	HistoryStruct		hs;
#endif
	RecordStruct		rs;

	HistoryQueueStruct	hq;
	HistoryQueueStruct	*hqTail;

	uint8			hitThisBlock;
	uint8			*shuffled;

	char			szFile[EH_PATH_MAX];
} ControlStruct;

/*
 * Error handling routines
 */
extern void eh_fatal(char *szFmt, ...);
extern void eh_logAssertion(char *szFile, int nLine);

/*
 * glue.c
 * File access routines
 */
extern void eh_CloseFile (FileStruct *pfs);
extern bool eh_CreateLock (FileStruct *pfs);

extern bool eh_LockFileStruct (FileStruct *pfs, bool bCreate);

extern Filedesc_t eh_OpenFile (FileStruct *pfs);

extern bool eh_RandomlyByteLockRange (FileStruct *pfs, bool bCreate);
extern void eh_ReleaseLock (FileStruct *pfs);

extern uint8 eh_Read (Filedesc_t fd, void *buf, uint8 bytes);
extern uint8 eh_Write (Filedesc_t fd, void *buf, uint8 bytes);

extern int eh_SessionDisconnected (void);


extern int eh_Rmdir (char *szPath);


/*
 * mt.c
 * Mersenne Twister random number generator.
 */
extern void init_genrand (unsigned long s, bool isB);
extern void init_by_array (unsigned long init_key[],
				unsigned long key_length, bool isB);
extern unsigned long genrand_int32 (bool isB);
extern uint8 genrand_int64 (bool isB);
extern long genrand_int31 (bool isB);
extern double genrand_real1 (bool isB);
extern double genrand_real2 (bool isB);
extern double genrand_real3 (bool isB);
extern double genrand_res53 (bool isB);

extern uint8 number_of_flips (bool isB);
extern void advance_flips (uint8 flips, bool isB);

/*
 * Some RPG style random number generators.
 * We use two tables, A and B, such that A is for the main
 * program and B is used for any temporary rolls.  I.e., in
 * verification, we can read the seed and flips from the
 * block and use B to generate the in core copy of the
 * data block for verification.  This allows us to not
 * loose the stream for A, which we could save by
 * storing the seed and flips locally and then regenerating
 * after we validate the data.
 */
#define ROLL_D2(isB) (bool)(genrand_int31((isB)) % 2)
#define ROLL_D4(isB) (int)(genrand_int31((isB)) % 4)
#define ROLL_D6(isB) (int)(genrand_int31((isB)) % 6)
#define ROLL_2D6(isB) (ROLL_D6((isB) + ROLL_D6((isB))
#define ROLL_D8(isB) (int)(genrand_int31((isB)) % 8)
#define ROLL_D10(isB) (int)(genrand_int31((isB)) % 10)
#define ROLL_D20(isB) (int)(genrand_int31((isB)) % 20)
#define ROLL_D100(isB) (int)(genrand_int31((isB)) % 100)
#define ROLL_UINT8(N, isB) genrand_int64((isB)) % (uint8)(N)
#define ROLL_RANGE(M, N, TYPE, isB) \
		(M) + (TYPE)(genrand_int32((isB)) % (uint8)((N) - (M)))

/*
 * list.c
 * Linked list code for directory entries.
 */
extern void appendElement(HeadStruct *phs, DirEntryStruct *pds);
extern void destroyList(HeadStruct *phs);
extern DirEntryStruct *getNextElementOf(HeadStruct *phs, uint8 uId);
extern bool isEmptyList(HeadStruct *phs);
extern void newHead(HeadStruct *phs);
extern void newList(HeadStruct *phs);

/*
 * dirent.c
 */
extern bool isDir(char *path);
extern bool isFile(char *path);

extern DirEntryStruct *findDirEntryOf (HeadStruct *phs, FindDirEnum fd,
					uint8 uToFind, uint8 *puCurr);

extern bool getDirEntriesOf (HeadStruct *phs, DirTypeEnum dte, char *szPath);

extern DirEntryStruct *findActiveFile (ControlStruct *pcs);

extern bool newBaseVerifyDirEntryOf (char *szPath, char *szOldPath,
					char *szBase, char *szOldBase,
					bool bDir);
extern bool pullBaseOutOf (char *szFileName, char *szOldPath,
				char *szOldBase);

extern void printDirEntriesOf (HeadStruct *phs, char *szBase);


extern void verifyBase (char *szBase);



/*
 * io.c
 */
extern char *allocateBlockData (uint8 blockSize, char *szFile, int iLine);
extern uint8 *allocateShuffled (uint8 len, char *szFile, int iLine);

extern void defaultStructCopy (DefaultsStruct *pdsDest,
				DefaultsStruct *pdsSource);

extern void initControlStruct (ControlStruct *pcs);

extern void printBlock (BlockStruct bs, uint8 number, uint8 max);
extern void printDFCB (DataFileControlBlock dfcb);
extern void printHistory (HistoryStruct hs, uint8 number);
extern void printRecord (RecordStruct rs, uint8 number);

extern int readBlock (Filedesc_t fd, BlockStruct *pout);
extern int readDFCB (Filedesc_t fd, DataFileControlBlock *pout);
extern int readHistory (Filedesc_t fd, HistoryStruct *pout);
extern uint8 readRecord (Filedesc_t fd, RecordStruct *prs);

extern int writeBlock (Filedesc_t fd, BlockStruct *pin);
extern int writeDFCB (Filedesc_t fd, DataFileControlBlock *pin);
extern int writeHistory (Filedesc_t fd, HistoryStruct *pin);
extern uint8 writeRecord (Filedesc_t fd, RecordStruct *prs);

extern void verifyControlStruct (ControlStruct *pcs);

/*
 * actions.c
 */
extern char *actionStringOf (ActionEnum ae);
extern bool decideOnAction (ControlStruct *pcs);
extern bool verifyTheRecord (ControlStruct *pcs);
extern char *useStringOf (UseEnum st);
extern void verifyActionWeightsOf (ActionWeightsStruct actions,
					char *szFile, int iLine);

extern void unwindHistory (ControlStruct *pcs);
extern HistoryQueueStruct *windHistory (ControlStruct *pcs,
					char *szLine, int iLine);


/*
 * endy.c
 */
extern void byteSwapData (char *pout, char *pin, uint8 blockSize);

/*
 * state.c
 */
extern void cleanupState (ControlStruct *pcs);
extern void initializeState (ControlStruct *pcs);
extern char *stateStringOf (StateEnum st);
extern void transitStates (ControlStruct *pcs);

extern PathStackStruct *buildPathStackOf (ControlStruct *pcs,
				PathStackStruct *stack, FindDirEnum fd,
				uint8 uToFind, uint8 *puDepth);
extern void freePathStackOf (PathStackStruct *stack);



#endif
