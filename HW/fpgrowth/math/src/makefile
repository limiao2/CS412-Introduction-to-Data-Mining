#-----------------------------------------------------------------------
# File    : makefile
# Contents: build math utility modules (on Unix systems)
# Author  : Christian Borgelt
# History : 2002.07.04 file created from makefile in util directory
#           2003.05.19 intexp, choose, zeta, quantile, and normd added
#           2003.08.15 module radfn added
#           2008.03.14 main programs added, quantile.c split/removed
#           2008.03.17 gamma distribution functions added
#           2010.10.08 changed standard from -ansi to -std=c99
#           2011.07.22 module ruleval added
#-----------------------------------------------------------------------
SHELL    = /bin/bash
THISDIR  = ../../math/src
UTILDIR  = ../../util/src
TRACTDIR = ../../tract/src

CC       = gcc -std=c99
# CC       = g++
CFBASE   = -Wall -Wextra -Wno-unused-parameter -Wconversion \
           -pedantic -c $(ADDFLAGS)
CFLAGS   = $(CFBASE) -DNDEBUG -O3 -funroll-loops
# CFLAGS   = $(CFBASE) -g

LD       = gcc
LDFLAGS  = $(ADDFLAGS)
LIBS     = -lm

GAMMA    = gammapdf gammacdf gammaqtl
NORMAL   = normpdf  normcdf  normqtl
CHI2     = chi2pdf  chi2cdf  chi2qtl
PRGS     = $(GAMMA) $(NORMAL) $(CHI2) gamma choose zeta

#-----------------------------------------------------------------------
# Build Programs
#-----------------------------------------------------------------------
all:        $(PRGS)

gamma:      gammain.o makefile
	$(LD) $(LDFLAGS) gammain.o $(LIBS) -o $@

choose:     chsmain.o makefile
	$(LD) $(LDFLAGS) chsmain.o $(LIBS) -o $@

zeta:       zetamain.o makefile
	$(LD) $(LDFLAGS) zetamain.o $(LIBS) -o $@

gammapdf:   gammapdf.o makefile
	$(LD) $(LDFLAGS) gammapdf.o $(LIBS) -o $@

gammacdf:   gammacdf.o makefile
	$(LD) $(LDFLAGS) gammacdf.o $(LIBS) -o $@

gammaqtl:   gammaqtl.o normal.o makefile
	$(LD) $(LDFLAGS) normal.o gammaqtl.o $(LIBS) -o $@

normpdf:    normpdf.o makefile
	$(LD) $(LDFLAGS) normpdf.o $(LIBS) -o $@

normcdf:    normcdf.o makefile
	$(LD) $(LDFLAGS) normcdf.o $(LIBS) -o $@

normqtl:    normqtl.o gamma.o makefile
	$(LD) $(LDFLAGS) gamma.o normqtl.o $(LIBS) -o $@

chi2pdf:    chi2pdf.o gamma.o makefile
	$(LD) $(LDFLAGS) gamma.o chi2pdf.o $(LIBS) -o $@

chi2cdf:    chi2cdf.o gamma.o makefile
	$(LD) $(LDFLAGS) gamma.o chi2cdf.o $(LIBS) -o $@

chi2qtl:    chi2qtl.o gammall.o normal.o makefile
	$(LD) $(LDFLAGS) gammall.o normal.o chi2qtl.o $(LIBS) -o $@

#-----------------------------------------------------------------------
# Programs
#-----------------------------------------------------------------------
gammain.o:  gamma.h
gammain.o:  gamma.c makefile
	$(CC) $(CFLAGS) -DGAMMA_MAIN gamma.c -o $@

chsmain.o:  choose.h
chsmain.o:  choose.c makefile
	$(CC) $(CFLAGS) -DCHOOSE_MAIN choose.c -o $@

zetamain.o: zeta.h
zetamain.o: zeta.c makefile
	$(CC) $(CFLAGS) -DZETA_MAIN zeta.c -o $@

gammapdf.o: gamma.h
gammapdf.o: gamma.c makefile
	$(CC) $(CFLAGS) -DGAMMAPDF_MAIN gamma.c -o $@

gammacdf.o: gamma.h
gammacdf.o: gamma.c makefile
	$(CC) $(CFLAGS) -DGAMMACDF_MAIN gamma.c -o $@

gammaqtl.o: gamma.h
gammaqtl.o: gamma.c makefile
	$(CC) $(CFLAGS) -DGAMMAQTL_MAIN gamma.c -o $@

normpdf.o:  normal.h
normpdf.o:  normal.c makefile
	$(CC) $(CFLAGS) -DNORMPDF_MAIN normal.c -o $@

normcdf.o:  normal.h
normcdf.o:  normal.c makefile
	$(CC) $(CFLAGS) -DNORMCDF_MAIN normal.c -o $@

normqtl.o:  normal.h
normqtl.o:  normal.c makefile
	$(CC) $(CFLAGS) -DNORMQTL_MAIN normal.c -o $@

chi2pdf.o:  chi2.h
chi2pdf.o:  chi2.c makefile
	$(CC) $(CFLAGS) -DCHI2PDF_MAIN chi2.c -o $@

chi2cdf.o:  chi2.h
chi2cdf.o:  chi2.c makefile
	$(CC) $(CFLAGS) -DCHI2CDF_MAIN chi2.c -o $@

chi2qtl.o:  chi2.h
chi2qtl.o:  chi2.c makefile
	$(CC) $(CFLAGS) -DCHI2QTL_MAIN chi2.c -o $@

#-----------------------------------------------------------------------
# Mathematical Functions
#-----------------------------------------------------------------------
intexp.o:   intexp.h
intexp.o:   intexp.c makefile
	$(CC) $(CFLAGS) intexp.c -o $@

choose.o:   choose.h
choose.o:   choose.c makefile
	$(CC) $(CFLAGS) choose.c -o $@

zeta.o:     zeta.h
zeta.o:     zeta.c makefile
	$(CC) $(CFLAGS) zeta.c -o $@

gamma.o:    gamma.h
gamma.o:    gamma.c makefile
	$(CC) $(CFLAGS) gamma.c -o $@

gammall.o:  gamma.h
gammall.o:  gamma.c makefile
	$(CC) $(CFLAGS) -DGAMMAQTL gamma.c -o $@

normal.o:   normal.h
normal.o:   normal.c makefile
	$(CC) $(CFLAGS) normal.c -o $@

chi2.o:     chi2.h
chi2.o:     chi2.c makefile
	$(CC) $(CFLAGS) chi2.c -o $@

chi2all.o:  chi2.h gamma.h normal.h
chi2all.o:  chi2.c makefile
	$(CC) $(CFLAGS) -DCHI2QTL chi2.c -o $@

radfn.o:    radfn.h gamma.h
radfn.o:    radfn.c makefile
	$(CC) $(CFLAGS) radfn.c -o $@

ruleval.o:  ruleval.h gamma.h $(UTILDIR)/arrays.h $(TRACTDIR)/tract.h
ruleval.o:  ruleval.c makefile
	$(CC) $(CFLAGS) -I$(UTILDIR) -I$(TRACTDIR) ruleval.c -o $@

#-----------------------------------------------------------------------
# Source Distribution Packages
#-----------------------------------------------------------------------
dist:
	$(MAKE) clean
	cd ../..; rm -f math.zip math.tar.gz; \
        zip -rq math.zip    math/{src,ex,doc}; \
        tar cfz math.tar.gz math/{src,ex,doc}

#-----------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------
install:
	cp $(PRGS) $(HOME)/bin

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
clean:
	rm -f $(PRGS) *.o *~ *.flc core
