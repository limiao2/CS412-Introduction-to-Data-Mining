/*----------------------------------------------------------------------
  File    : fpgrowth.h
  Contents: fpgrowth algorithm for finding frequent item sets
  Author  : Christian Borgelt
  History : 2011.08.22 file created
            2011.09.21 available variants and modes reorganized
            2014.08.07 association rule generation/evaluation added
            2014.08.19 adapted to modified item set reporter interface
            2014.08.21 parameter 'body' added to function fpgrowth()
            2014.08.28 functions fpg_data() and fpg_repo() added
----------------------------------------------------------------------*/
#ifndef __FPGROWTH__
#define __FPGROWTH__
#include "tract.h"
#ifndef ISR_CLOMAX
#define ISR_CLOMAX
#endif
#include "report.h"
#include "ruleval.h"
#include "istree.h"

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
/* --- evaluation measures --- */
/* most definitions in ruleval.h */
#define FPG_LDRATIO RE_FNCNT    /* binary log. of support quotient */
#define FPG_INVBXS  IST_INVBXS  /* inval. eval. below exp. supp. */

/* --- aggregation modes --- */
#define FPG_NONE    IST_NONE    /* no aggregation (use first value) */
#define FPG_FIRST   IST_FIRST   /* no aggregation (use first value) */
#define FPG_MIN     IST_MIN     /* minimum of measure values */
#define FPG_MAX     IST_MAX     /* maximum of measure values */
#define FPG_AVG     IST_AVG     /* average of measure values */

/* --- algorithm variants --- */
#define FPG_SIMPLE  0           /* simple  nodes (parent/link) */
#define FPG_COMPLEX 1           /* complex nodes (children/sibling) */
#define FPG_SINGLE  2           /* top-down processing on single tree */
#define FPG_TOPDOWN 3           /* top-down processing of the tree */

/* --- operation modes --- */
#define FPG_FIM16   0x001f      /* use 16 items machine (bit rep.) */
#define FPG_PERFECT 0x0020      /* perfect extension pruning */
#define FPG_REORDER 0x0040      /* reorder items in cond. databases */
#define FPG_TAIL    0x0080      /* head union tail pruning */
#define FPG_DEFAULT (FPG_PERFECT|FPG_REORDER|FPG_TAIL|FPG_FIM16)
#ifdef NDEBUG
#define FPG_NOCLEAN 0x8000      /* do not clean up memory */
#else                           /* in function fpgrowth() */
#define FPG_NOCLEAN 0           /* in debug version */
#endif                          /* always clean up memory */
#define FPG_VERBOSE INT_MIN     /* verbose message output */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern int fpg_data (TABAG *tabag, int target, SUPP smin, ITEM zmin,
                     int eval, int algo, int mode, int sort);
extern int fpg_repo (ISREPORT *report, int target,
                     int eval, double thresh, int algo, int mode);
extern int fpgrowth (TABAG *tabag, int target, SUPP smin, SUPP body,
                     double conf, int eval, int agg, double thresh,
                     ITEM prune, int algo, int mode,
                     int order, ISREPORT *report);
#endif
