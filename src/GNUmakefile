# $Id$
#
# Copyright (c) 2001 Network Appliance, Inc.
# All rights reserved.

include ../Defs

all: build

BINS=elham$(EXE) datadump$(EXE) histdump$(EXE) metadump$(EXE)

OBJS := actions.o dirent.o endy.o glue.o io.o list.o mt.o state.o elham.o

LIBS :=-lm
ifeq ($(UNIX),yes)
LIBS := $(LIBS)
endif

$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

$(BINS): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

include ../Rules
