/*----------------------------------------------------------------------
  File    : fpgpsp.c
  Contents: generate or estimate a pattern spectrum (FP-growth)
  Author  : Christian Borgelt
  History : 2015.08.28 file created
----------------------------------------------------------------------*/
#ifndef __FPGPSP__
#define __FPGPSP__
#include "fpgrowth.h"

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
/* --- surrogate data methods --- */
#define SUR_IDENTITY    0       /* identity (keep original data) */
#define SUR_RANDOM      1       /* spike time randomization */
#define SUR_SWAP        2       /* permutation by pair swaps */
#define SUR_SHUFFLE     3       /* shuffle table-derived data */

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef void PRGREPFN (long cnt, void *data);
                                /* progress reporting function */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern PATSPEC* fpg_genpsp (TABAG *tabag, int target, SUPP supp,
                            ITEM zmin, ITEM zmax, int algo, int mode,
                            size_t cnt, int surr, long seed,
                            int cpus, PRGREPFN *rep, void* data);

extern PATSPEC* fpg_estpsp (TABAG *tabag, int target, SUPP supp,
                            ITEM zmin, ITEM zmax, size_t equiv,
                            double alpha, size_t smpls, long seed);
#endif
