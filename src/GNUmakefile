# $Id$
#
# Copyright (c) 2001 Network Appliance, Inc.
# All rights reserved.

include ../Defs

all: build

BINS=elham$(EXE) datadump$(EXE) histdump$(EXE) metadump$(EXE)

OBJS := actions.o dirent.o endy.o glue.o io.o list.o mt.o state.o

LIBS :=-lm
ifeq ($(UNIX),yes)
LIBS := $(LIBS)
endif

actions.o: actions.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

dirent.o: dirent.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

endy.o: endy.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

glue.o: glue.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

io.o: io.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

list.o: list.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

mt.o: mt.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

state.o: state.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

elham.o: elham.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

datadump.o: datadump.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

histdump.o: histdump.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

metadump.o: metadump.c
	$(CC) $(CFLAGS) -o $@ -c $< -D_FILE_OFFSET_BITS=$(LARGEST_OFFSET)

elham: $(OBJS) elham.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

histdump: $(OBJS) histdump.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

datadump: $(OBJS) datadump.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

metadump: $(OBJS) metadump.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

include ../Rules
