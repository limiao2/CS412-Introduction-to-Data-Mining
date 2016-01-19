#-----------------------------------------------------------------------
# File    : tract.mak
# Contents: build item and transaction management (on Windows systems)
# Author  : Christian Borgelt
# History : 2008.10.05 file created from apriori makefile
#           2011.05.06 changed to double support reporting/recording
#           2011.08.29 main program fim16 added (mainly for testing)
#           2012.07.27 module tract with write functions added (trawr)
#           2013.04.04 added external modules and tract/train main prgs.
#-----------------------------------------------------------------------
THISDIR  = ..\..\tract\src
UTILDIR  = ..\..\util\src
MATHDIR  = ..\..\math\src

CC       = cl.exe
DEFS     = /D WIN32 /D NDEBUG /D _CONSOLE /D _CRT_SECURE_NO_WARNINGS
CFLAGS   = /nologo /W3 /O2 /GS- $(DEFS) /c $(ADDFLAGS)
INCS     = /I $(UTILDIR) /I $(MATHDIR)

LD       = link.exe
LDFLAGS  = /nologo /subsystem:console /incremental:no
LIBS     = 

HDRS     = $(UTILDIR)\arrays.h    $(UTILDIR)\memsys.h     \
           $(UTILDIR)\symtab.h    $(UTILDIR)\escape.h     \
           $(UTILDIR)\tabread.h   $(UTILDIR)\tabwrite.h   \
           $(UTILDIR)\scanner.h  \
           tract.h patspec.h clomax.h report.h fim16.h
OBJS     = $(UTILDIR)\arrays.obj  $(UTILDIR)\memsys.obj   \
           $(UTILDIR)\idmap.obj   $(UTILDIR)\escape.obj   \
           $(UTILDIR)\tabread.obj $(UTILDIR)\tabwrite.obj \
           $(UTILDIR)\scform.obj \
           taread.obj patspec.obj clomax.obj repcm.obj
PRGS     = fim16 tract train

#-----------------------------------------------------------------------
# Build Module
#-----------------------------------------------------------------------
all:          $(PRGS)

fim16:        $(OBJS) ma16main.obj tract.mak
	$(LD) $(LDFLAGS) $(OBJS) m16main.obj $(LIBS) /out:$@

tract:        $(OBJS) tramain.obj tract.mak
	$(LD) $(LDFLAGS) $(OBJS) tramain.obj $(LIBS) /out:$@

train:        $(OBJS) trnmain.obj $(UTILDIR)\tabwrite.obj tract.mak
	$(LD) $(LDFLAGS) $(OBJS) $(UTILDIR)\tabwrite.obj \
              trnmain.obj $(LIBS) /out:$@

psp:          $(UTILDIR)/tabwrite.o $(UTILDIR)/escape.o $(ADDOBJS)
psp:          pspmain.o tract.mak
	$(LD) $(LDFLAGS) $(UTILDIR)/tabwrite.o $(UTILDIR)/escape.o \
              $(ADDOBJS) pspmain.o $(LIBS) /Fo$@

#-----------------------------------------------------------------------
# Main Programs
#-----------------------------------------------------------------------
tramain.obj:  $(HDRS)
tramain.obj:  tract.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TA_MAIN tract.c /Fo$@

trnmain.obj:  $(HDRS)
trnmain.obj:  train.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TRN_MAIN train.c /Fo$@

m16main.obj:  $(HDRS)
m16main.obj:  fim16.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D M16_MAIN fim16.c /Fo$@

pspmain.obj:  $(HDRS) $(UTILDIR)\tabwrite.h
pspmain.obj:  patspec.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D PSP_MAIN patspec.c /Fo$@

#-----------------------------------------------------------------------
# Item and Transaction Management
#-----------------------------------------------------------------------
tract.obj:    $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h tract.h
tract.obj:    tract.c tract.mak
	$(CC) $(CFLAGS) $(INCS) tract.c /Fo$@

tasurr.obj:   $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h tract.h
tasurr.obj:   tract.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TA_SURR tract.c /Fo$@

taread.obj:   $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h tract.h
taread.obj:   tract.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TA_READ tract.c /Fo$@

tarw.obj:     $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h $(UTILDIR)\tabwrite.h tract.h
tarw.obj:     tract.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TA_READ /D TA_WRITE tract.c /Fo$@

tars.obj:     $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h tract.h
tars.obj:     tract.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TA_READ /D TA_SURR tract.c /Fo$@

tard.obj:     $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h tract.h
tard.obj:     tract.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TA_READ /D SUPP=double tract.c /Fo$@

tatree.obj:   $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h tract.h
tatree.obj:   tract.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TA_READ /D TATREEFN tract.c /Fo$@

#-----------------------------------------------------------------------
# Train Management
#-----------------------------------------------------------------------
train.obj:    $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h train.h tract.h
train.obj:    train.c tract.mak
	$(CC) $(CFLAGS) $(INCS) train.c /Fo$@

trnsurr.obj:  $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h train.h tract.h
trnsurr.obj:  train.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TRN_SURR train.c /Fo$@

trnread.obj:  $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h train.h tract.h
trnread.obj:  train.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TRN_READ train.c /Fo$@

trnrw.obj:    $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h $(UTILDIR)\tabwrite.h train.h tract.h
trnrw.obj:    train.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TRN_READ /D TRN_WRITE train.c /Fo$@

trnrs.obj:    $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h train.h tract.h
trnrs.obj:    train.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TRN_READ /D TRN_SURR train.c /Fo$@

trnrd.obj:    $(UTILDIR)\arrays.h  $(UTILDIR)\symtab.h \
              $(UTILDIR)\tabread.h train.h tract.h
trnrd.obj:    train.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D TRN_READ /D SUPP=double \
              train.c /Fo$@

#-----------------------------------------------------------------------
# Frequent Item Set Mining (with at most 16 items)
#-----------------------------------------------------------------------
fim16.obj:    $(HDRS)
fim16.obj:    fim16.c tract.mak
	$(CC) $(CFLAGS) $(INCS) fim16.c /Fo$@

#-----------------------------------------------------------------------
# Pattern Statistics Management
#-----------------------------------------------------------------------
patspec.obj:  patspec.h $(UTILDIR)\tabwrite.h
patspec.obj:  patspec.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D PSP_REPORT patspec.c /Fo$@

pspdbl.obj:   patspec.h $(UTILDIR)\tabwrite.h
pspdbl.obj:   patspec.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D SUPP=double /D PSP_REPORT \
              patspec.c /Fo$@

pspest.obj:   patspec.h $(UTILDIR)\tabwrite.h
pspest.obj:   patspec.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D PSP_ESTIM /D PSP_TRAIN /D PSP_REPORT \
	      patspec.c /Fo$@

#-----------------------------------------------------------------------
# Prefix Tree Management for Closed/Maximal Filtering
#-----------------------------------------------------------------------
clomax.obj:   clomax.h $(UTILDIR)\arrays.h $(UTILDIR)\memsys.h
clomax.obj:   clomax.c tract.mak
	$(CC) $(CFLAGS) $(INCS) clomax.c /Fo$@

cmdbl.obj:    clomax.h $(UTILDIR)\arrays.h $(UTILDIR)\memsys.h
cmdbl.obj:    clomax.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D RSUPP=double clomax.c /Fo$@

#-----------------------------------------------------------------------
# Item Set Reporter Management
#-----------------------------------------------------------------------
report.obj:   report.h tract.h $(UTILDIR)\symtab.h
report.obj:   report.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D ISR_PATSPEC report.c /Fo$@

repdbl.obj:   report.h tract.h $(UTILDIR)\symtab.h
repdbl.obj:   report.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D RSUPP=double /D ISR_PATSPEC \
              report.c /Fo$@

repcm.obj:    report.h tract.h $(UTILDIR)\symtab.h
repcm.obj:    report.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D ISR_PATSPEC /D ISR_CLOMAX \
              report.c /Fo$@

repcmd.obj:   report.h tract.h $(UTILDIR)\symtab.h
repcmd.obj:   report.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D RSUPP=double /D ISR_PATSPEC \
              /D ISR_CLOMAX report.c /Fo$@

#-----------------------------------------------------------------------
# Rule Generation Tree Management
#-----------------------------------------------------------------------
rulegen.obj:  rulegen.h tract.h $(UTILDIR)\arrays.h $(UTILDIR)\memsys.h
rulegen.obj:  rulegen.c tract.mak
	$(CC) $(CFLAGS) $(INCS) rulegen.c /Fo$@

rgrfn.obj:    rulegen.h tract.h $(UTILDIR)\arrays.h $(UTILDIR)\memsys.h
rgrfn.obj:    rulegen.c tract.mak
	$(CC) $(CFLAGS) $(INCS) /D RG_REPOFN rulegen.c /Fo$@

#-----------------------------------------------------------------------
# External Modules
#-----------------------------------------------------------------------
$(UTILDIR)\arrays.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak arrays.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\memsys.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak memsys.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\idmap.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak idmap.obj    ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\escape.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak escape.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\tabread.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak tabread.obj  ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\tabwrite.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak tabwrite.obj ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\scform.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak scform.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(MATHDIR)\ruleval.obj:
	cd $(MATHDIR)
        $(MAKE) /f math.mak ruleval.obj  ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(MATHDIR)\gamma.obj:
	cd $(MATHDIR)
        $(MAKE) /f math.mak gamma.obj    ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(MATHDIR)\chi2.obj:
	cd $(MATHDIR)
        $(MAKE) /f math.mak gamma.obj    ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
localclean:
	-@erase /Q *~ *.obj *.idb *.pch $(PRGS)

clean:
	$(MAKE) /f tract.mak localclean
	cd $(MATHDIR)
	$(MAKE) /f math.mak clean
	cd $(UTILDIR)
	$(MAKE) /f util.mak clean
	cd $(THISDIR)
