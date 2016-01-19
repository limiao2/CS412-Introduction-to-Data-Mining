#-----------------------------------------------------------------------
# File    : makefile
# Contents: build utility modules (on Unix systems)
# Author  : Christian Borgelt
# History : 2000.11.04 file created from makefile in table directory
#           2003.06.05 module params added
#           2003.08.12 module nstats added
#           2004.12.10 module memsys added
#           2008.08.01 adapted to name changes of arrays and lists
#           2008.08.18 adapted to main functions of arrays and lists
#           2008.08.22 module escape added, test program trdtest added
#           2010.10.07 changed c standard from -ansi to -std=c99
#           2010.10.08 module tabwrite added
#           2013.03.20 extended the requested warnings in CFBASE
#           2015.04.15 module strlist added
#-----------------------------------------------------------------------
SHELL   = /bin/bash
THISDIR = ../../util/src

CC      = gcc -std=c99
# CC      = g++
CFBASE  = -Wall -Wextra -Wno-unused-parameter -Wconversion \
          -pedantic -c $(ADDFLAGS)
CFLAGS  = $(CFBASE) -DNDEBUG -O3 -funroll-loops
# CFLAGS  = $(CFBASE) -g
# CFLAGS  = $(CFBASE) -g -DSTORAGE

LD      = gcc
# LD      = g++
LDFLAGS = $(ADDFLAGS)
LIBS    = 

# ADDOBJS  = $(UTILDIR)/storage.o

PRGS    = sortargs listtest trdtest

#-----------------------------------------------------------------------
# Build Programs
#-----------------------------------------------------------------------
all:        $(PRGS)

sortargs:   sortargs.o makefile
	$(LD) $(LDFLAGS) $(LIBS) sortargs.o -o $@

listtest:   listtest.o makefile
	$(LD) $(LDFLAGS) $(LIBS) listtest.o -o $@

trdtest:    trdtest.o escape.o arrays.o idmap.o makefile
	$(LD) $(LDFLAGS) $(LIBS) escape.o arrays.o idmap.o trdtest.o -o $@

#-----------------------------------------------------------------------
# Programs
#-----------------------------------------------------------------------
sortargs.o: arrays.h fntypes.h
sortargs.o: arrays.c makefile
	$(CC) $(CFLAGS) -DARRAYS_MAIN arrays.c -o $@

listtest.o: lists.h fntypes.h
listtest.o: lists.c makefile
	$(CC) $(CFLAGS) -DLISTS_MAIN lists.c -o $@

trdtest.o:  tabread.h
trdtest.o:  tabread.c makefile
	$(CC) $(CFLAGS) -DTRD_MAIN tabread.c -o $@

#-----------------------------------------------------------------------
# Array Operations
#-----------------------------------------------------------------------
arrays.o:   arrays.h fntypes.h
arrays.o:   arrays.c makefile
	$(CC) $(CFLAGS) arrays.c -o $@

#-----------------------------------------------------------------------
# List Operations
#-----------------------------------------------------------------------
lists.o:    lists.h fntypes.h
lists.o:    lists.c makefile
	$(CC) $(CFLAGS) lists.c -o $@

#-----------------------------------------------------------------------
# String List Operations
#-----------------------------------------------------------------------
strlist.o:  strlist.h
strlist.o:  strlist.c makefile
	$(CC) $(CFLAGS) strlist.c -o $@

#-----------------------------------------------------------------------
# Memory Management System for Objects of Equal Size
#-----------------------------------------------------------------------
memsys.o:   memsys.h
memsys.o:   memsys.c makefile
	$(CC) $(CFLAGS) memsys.c -o $@

#-----------------------------------------------------------------------
# Symbol Table Management
#-----------------------------------------------------------------------
symtab.o:   symtab.h fntypes.h
symtab.o:   symtab.c makefile
	$(CC) $(CFLAGS) symtab.c -o $@

idmap.o:    symtab.h fntypes.h arrays.h
idmap.o:    symtab.c makefile
	$(CC) $(CFLAGS) -DIDMAPFN symtab.c -o $@

#-----------------------------------------------------------------------
# Random Number Generator
#-----------------------------------------------------------------------
random.o:   random.h
random.o:   random.c makefile
	$(CC) $(CFLAGS) random.c -o $@

#-----------------------------------------------------------------------
# Numerical Statistics Management
#-----------------------------------------------------------------------
nstats.o:   nstats.h
nstats.o:   nstats.c makefile
	$(CC) $(CFLAGS) nstats.c -o $@

nst_pars.o: nstats.h
nst_pars.o: nstats.c makefile
	$(CC) $(CFLAGS) -DNST_PARSE nstats.c -o $@

#-----------------------------------------------------------------------
# Escape Character Handling
#-----------------------------------------------------------------------
escape.o:   escape.h
escape.o:   escape.c makefile
	$(CC) $(CFLAGS) escape.c -o $@

#-----------------------------------------------------------------------
# Table Reader and Writer Management
#-----------------------------------------------------------------------
tabread.o:  tabread.h escape.h
tabread.o:  tabread.c makefile
	$(CC) $(CFLAGS) tabread.c -o $@

tabwrite.o: tabwrite.h escape.h
tabwrite.o: tabwrite.c makefile
	$(CC) $(CFLAGS) tabwrite.c -o $@

#-----------------------------------------------------------------------
# Scanner
#-----------------------------------------------------------------------
scform.o:   scanner.h
scform.o:   scanner.c makefile
	$(CC) $(CFLAGS) scanner.c -o $@

scanner.o:  scanner.h
scanner.o:  scanner.c makefile
	$(CC) $(CFLAGS) -DSCN_SCAN scanner.c -o $@

#-----------------------------------------------------------------------
# Command Line Parameter Retrieval
#-----------------------------------------------------------------------
params.o:   params.h
params.o:   params.c makefile
	$(CC) $(CFLAGS) params.c -o $@

#-----------------------------------------------------------------------
# Interrupt Signal Handling
#-----------------------------------------------------------------------
sigint.o:   sigint.h
sigint.o:   sigint.c makefile
	$(CC) $(CFLAGS) sigint.c -o $@

#-----------------------------------------------------------------------
# Storage Debugging Utility
#-----------------------------------------------------------------------
storage.o:  storage.h
storage.o:  storage.c makefile
	$(CC) $(CFLAGS) storage.c -o $@

#-----------------------------------------------------------------------
# Source Distribution Packages
#-----------------------------------------------------------------------
dist:
	$(MAKE) clean
	cd ../..; rm -f util.zip util.tar.gz; \
        zip -rq util.zip    util/{src,doc}; \
        tar cfz util.tar.gz util/{src,doc}

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
clean:
	rm -f $(PRGS) *.o *~ *.flc core
