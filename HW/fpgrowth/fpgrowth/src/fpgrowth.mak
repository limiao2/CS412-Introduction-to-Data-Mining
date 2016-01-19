#-----------------------------------------------------------------------
# File    : fpgrowth.mak
# Contents: build fpgrowth program (on Windows systems)
# Author  : Christian Borgelt
# History : 2004.11.21 file created from eclat.mak
#           2006.07.20 adapted to Visual Studio 8
#           2010.08.22 module escape added (for module tabread)
#           2011.09.20 external module fim16 added (16 items machine)
#           2014.08.21 extended by module istree from apriori source
#-----------------------------------------------------------------------
THISDIR  = ..\..\fpgrowth\src
UTILDIR  = ..\..\util\src
MATHDIR  = ..\..\math\src
TRACTDIR = ..\..\tract\src
APRIDIR  = ..\..\apriori\src

CC       = cl.exe
DEFS     = /D WIN32 /D NDEBUG /D _CONSOLE /D _CRT_SECURE_NO_WARNINGS
CFLAGS   = /nologo /W3 /O2 /GS- $(DEFS) /c $(ADDFLAGS)
INCS     = /I $(UTILDIR) /I $(TRACTDIR) /I $(MATHDIR) /I $(APRIDIR)

LD       = link.exe
LDFLAGS  = /nologo /subsystem:console /incremental:no
LIBS     = 

HDRS     = $(UTILDIR)\memsys.h     $(UTILDIR)\arrays.h     \
           $(UTILDIR)\symtab.h     $(UTILDIR)\escape.h     \
           $(UTILDIR)\tabread.h    $(UTILDIR)\tabwrite.h   \
           $(UTILDIR)\scanner.h    $(MATHDIR)\gamma.h      \
           $(MATHDIR)\chi2.h       $(MATHDIR)\ruleval.h    \
           $(TRACTDIR)\tract.h     $(TRACTDIR)\patspec.h   \
           $(TRACTDIR)\clomax.h    $(TRACTDIR)\report.h    \
           $(TRACTDIR)\fim16.h     $(APRIDIR)\istree.h     \
           fpgrowth.h
OBJS     = $(UTILDIR)\memsys.obj   $(UTILDIR)\arrays.obj   \
           $(UTILDIR)\idmap.obj    $(UTILDIR)\escape.obj   \
           $(UTILDIR)\tabread.obj  $(UTILDIR)\tabwrite.obj \
           $(UTILDIR)\scform.obj   $(MATHDIR)\gamma.obj    \
           $(MATHDIR)\chi2.obj     $(MATHDIR)\ruleval.obj  \
           $(TRACTDIR)\clomax.obj  $(TRACTDIR)\repcm.obj   \
           $(TRACTDIR)\fim16.obj   $(APRIDIR)\istree.obj

FPGOBJS  = $(OBJS)                 $(TRACTDIR)\taread.obj  \
           $(TRACTDIR)\patspec.obj fpgmain.obj
PSPOBJS  = $(OBJS)                 $(UTILDIR)\random.obj   \
           $(UTILDIR)\sigint.obj   $(TRACTDIR)\tars.obj    \
           $(TRACTDIR)\pspest.obj  fpgrowth.obj pspmain.obj

PRGS     = fpgrowth.exe fpgpsp.exe

#-----------------------------------------------------------------------
# Build Program
#-----------------------------------------------------------------------
all:         $(PRGS)

fpgrowth.exe: $(FPGOBJS) fpgrowth.mak
	$(LD) $(LDFLAGS) $(FPGOBJS) $(LIBS) /out:$@

fpgpsp.exe:   $(PSPOBJS) fpgrowth.mak
	$(LD) $(LDFLAGS) $(PSPOBJS) $(LIBS) /out:$@

#-----------------------------------------------------------------------
# Main Programs
#-----------------------------------------------------------------------
fpgmain.obj:  $(HDRS) fpgrowth.c fpgrowth.mak
	$(CC) $(CFLAGS) $(INCS) /D FPG_MAIN fpgrowth.c /Fo$@

pspmain.obj:  $(HDRS) fpgpsp.c fpgrowth.mak
	$(CC) $(CFLAGS) $(INCS) /D FPGPSP_MAIN fpgpsp.c /Fo$@

#-----------------------------------------------------------------------
# FP-growth as a module
#-----------------------------------------------------------------------
fpgrowth.obj: $(HDRS) fpgrowth.h
fpgrowth.obj: fpgrowth.c fpgrowth.mak
	$(CC) $(CFLAGS) $(INCS) /D FPG_ABORT fpgrowth.c /Fo$@

#-----------------------------------------------------------------------
# Pattern Spectrum Functions
#-----------------------------------------------------------------------
fpgpsp.obj:   $(HDRS) $(UTILDIR)\fntypes.h fpgrowth.h fpgpsp.h
fpgpsp.obj:   fpgpsp.c fpgrowth.mak
	$(CC) $(CFLAGS) $(INCS) /D FPG_ABORT fpgpsp.c /Fo$@

#-----------------------------------------------------------------------
# External Modules
#-----------------------------------------------------------------------
$(UTILDIR)\memsys.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    memsys.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\arrays.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    arrays.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\idmap.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    idmap.obj    ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\escape.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    escape.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\tabread.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    tabread.obj  ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\tabwrite.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    tabwrite.obj ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\scform.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    scform.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\random.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    random.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(UTILDIR)\sigint.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak    sigint.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(MATHDIR)\ruleval.obj:
	cd $(MATHDIR)
        $(MAKE) /f math.mak    ruleval.obj  ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(MATHDIR)\gamma.obj:
	cd $(MATHDIR)
	$(MAKE) /f math.mak    gamma.obj    ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(MATHDIR)\chi2.obj:
	cd $(MATHDIR)
	$(MAKE) /f math.mak    chi2.obj     ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(TRACTDIR)\taread.obj:
	cd $(TRACTDIR)
	$(MAKE) /f tract.mak   taread.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(TRACTDIR)\tars.obj:
	cd $(TRACTDIR)
	$(MAKE) /f tract.mak   tars.obj     ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(TRACTDIR)\patspec.obj:
	cd $(TRACTDIR)
	$(MAKE) /f tract.mak   patspec.obj  ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(TRACTDIR)\pspest.obj:
	cd $(TRACTDIR)
	$(MAKE) /f tract.mak   pspest.obj  ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(TRACTDIR)\clomax.obj:
	cd $(TRACTDIR)
	$(MAKE) /f tract.mak   clomax.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(TRACTDIR)\repcm.obj:
	cd $(TRACTDIR)
	$(MAKE) /f tract.mak   repcm.obj    ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(TRACTDIR)\fim16.obj:
	cd $(TRACTDIR)
	$(MAKE) /f tract.mak   fim16.obj    ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)
$(APRIDIR)\istree.obj:
	cd $(APRIDIR)
	$(MAKE) /f apriori.mak istree.obj   ADDFLAGS="$(ADDFLAGS)"
	cd $(THISDIR)

#-----------------------------------------------------------------------
# Install
#-----------------------------------------------------------------------
install:
	-@copy *.exe ..\..\..\bin

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
localclean:
	-@erase /Q *~ *.obj *.idb *.pch $(PRGS)

clean:
	$(MAKE) /f fpgrowth.mak localclean
	cd $(APRIDIR)
	$(MAKE) /f apriori.mak localclean
	cd $(TRACTDIR)
	$(MAKE) /f tract.mak localclean
	cd $(MATHDIR)
	$(MAKE) /f math.mak clean
	cd $(UTILDIR)
	$(MAKE) /f util.mak clean
	cd $(THISDIR)
