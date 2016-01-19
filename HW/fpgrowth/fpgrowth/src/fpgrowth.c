/*----------------------------------------------------------------------
  File    : fpgrowth.c
  Contents: fpgrowth algorithm for finding frequent item sets
  Author  : Christian Borgelt
  History : 2004.11.21 file created from eclat.c
            2004.11.23 absolute/relative support output changed
            2004.12.09 filter added (binary logarithm of supp. quotient)
            2005.06.20 use of flag for "no item sorting" corrected
            2007.02.13 adapted to modified module tabread
            2008.01.22 special processing of chains added
            2008.05.02 default limit for maximal number of items removed
            2008.09.01 adapted to modified modules tract and report
            2008.09.02 pruning with perfect extensions added
            2008.09.09 more flexible information output control added
            2008.10.26 maximum item set size removed from recursion
            2008.10.31 adapted to changes in item set reporting
            2008.11.13 adapted to changes in transaction management
            2009.05.28 adapted to modified function tbg_filter()
            2009.10.15 adapted to item set counter in reporter
            2009.10.18 treatment of item chains simplified
            2009.10.19 closed and maximal item set mining added
            2010.02.08 adapted to new repository for filtering
            2010.03.03 copy pointer removed from structure FPNODE
            2010.03.04 memory system eliminated (now array of nodes)
            2010.03.17 head union tail pruning for maximal sets added
            2010.07.14 output file made optional (for benchmarking)
            2010.07.20 created projections are no longer reallocated
            2010.07.25 separate frequent pattern tree module removed
            2010.07.26 fixed frequent pattern tree for each recursion
            2010.07.27 variant with children/sibling nodes added
            2010.07.29 variant added that processes the tree top-down
            2010.07.30 simple node structure variant redesigned
            2010.08.03 complex node variant with item reordering added
            2010.08.05 top-down processing with single tree added
            2010.08.19 item selection file added as optional input
            2010.08.22 adapted to modified modules tabread and tract
            2010.08.30 bug in header table initialization fixed
            2010.10.15 adapted to modified interface of module report
            2010.11.24 adapted to modified error reporting (tract)
            2010.12.07 added some explicit type casts (for C++)
            2010.12.11 adapted to a generic error reporting function
            2010.12.20 adapted to function tbg_ifrqs() (filter problem)
            2011.03.20 optional integer transaction weights added
            2011.07.08 adapted to modified function tbg_recode()
            2011.08.17 adapted algorithm variants to finding generators
            2011.08.28 output of item set counters per size added
            2011.08.29 bug in rec_single() fixed (perfect extensions)
            2011.09.16 choice of item loop direction generalized
            2011.09.21 added use of 16-items machine for simple trees
            2011.09.27 bug in algorithm and mode checking fixed (hut)
            2012.04.09 bug in function rec_single() fixed (isr_xable())
            2012.05.25 masking of packed items added to proj_simple()
            2012.05.30 simple tree functions split w.r.t. use of fim16
            2012.06.19 use of 16-items machine for complex trees added
            2013.03.07 direction parameter added to sorting functions
            2013.04.02 adapted to type changes in module tract
            2013.10.15 checks of return code of isr_report() added
            2013.10.18 optional pattern spectrum collection added
            2013.11.12 item selection file changed to option -R#
            2014.05.12 option -F# added (support border for filtering)
            2014.08.07 association rule generation/evaluation added
            2014.08.21 adapted to modified item set reporter interface
            2014.08.28 functions fpg_data() and fpg_repo() added
            2014.10.24 changed from LGPL license to MIT license
------------------------------------------------------------------------
  Reference for the FP-growth algorithm:
    J. Han, H. Pei, and Y. Yin.
    Mining Frequent Patterns without Candidate Generation.
    Proc. 19th ACM SIGMOD Int. Conf. on Management of Data
    (SIGMOD'00, Dallas, TX), 1-12.
    ACM Press, New York, NY, USA 2000
  Top-down variant on single tree (option -Ad):
    Top-Down FP-Growth for Association Rule Mining
    K. Wang, L. Tang, J. Han, and J. Liu.
    Proc. 6th Pacific-Asia Conf. on Advances in Knowledge Discovery
    and Data Mining (PAKDD 2002, Taipei, Taiwan), 334-340.
    Springer-Verlag, London, United Kingdom 2002
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "memsys.h"
#ifndef ISR_PATSPEC
#define ISR_PATSPEC
#endif
#ifdef FPG_MAIN
#ifndef PSP_REPORT
#define PSP_REPORT
#endif
#ifndef TA_READ
#define TA_READ
#endif
#endif
#ifdef FPG_ABORT
#include "sigint.h"
#endif
#include "fpgrowth.h"
#include "fim16.h"
#ifdef FPG_MAIN
#include "error.h"
#endif
#ifdef STORAGE
#include "storage.h"
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define PRGNAME     "fpgrowth"
#define DESCRIPTION "find frequent item sets " \
                    "with the fpgrowth algorithm"
#define VERSION     "version 6.7 (2015.08.18)         " \
                    "(c) 2004-2015   Christian Borgelt"

/* --- error codes --- */
/* error codes   0 to  -4 defined in tract.h */
#define E_STDIN      (-5)       /* double assignment of stdin */
#define E_OPTION     (-6)       /* unknown option */
#define E_OPTARG     (-7)       /* missing option argument */
#define E_ARGCNT     (-8)       /* too few/many arguments */
#define E_TARGET     (-9)       /* invalid target type */
#define E_SIZE      (-10)       /* invalid item set size */
#define E_SUPPORT   (-11)       /* invalid item set support */
#define E_CONF      (-12)       /* invalid confidence */
#define E_MEASURE   (-13)       /* invalid evaluation measure */
#define E_AGGMODE   (-14)       /* invalid aggregation mode */
#define E_VARIANT   (-16)       /* invalid algorithm variant */
/* error codes -15 to -25 defined in tract.h */

#define COPYERR     ((TDNODE*)-1)

#ifndef QUIET                   /* if not quiet version, */
#define MSG         fprintf     /* print messages */
#define XMSG        if (mode & FPG_VERBOSE) fprintf
#define CLOCK(t)    ((t) = clock())
#else                           /* if quiet version, */
#define MSG(...)    ((void)0)   /* suppress messages */
#define XMSG(...)   ((void)0)
#define CLOCK(t)    ((void)0)
#endif

#define SEC_SINCE(t)  ((double)(clock()-(t)) /(double)CLOCKS_PER_SEC)

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct fpnode {         /* --- frequent pattern tree node --- */
  ITEM          id;             /* item/head identifier */
  SUPP          supp;           /* support (weight of transactions) */
  struct fpnode *parent;        /* parent node (preceding item) */
  struct fpnode *succ;          /* successor node with same item */
} FPNODE;                       /* (frequent pattern tree node) */

typedef struct {                /* --- freq. pat. tree node list --- */
  ITEM     item;                /* associated item (item base code) */
  SUPP     supp;                /* support (weight of transactions) */
  FPNODE   *list;               /* list of nodes with this item */
} FPHEAD;                       /* (frequent pattern tree head) */

typedef struct {                /* --- frequent pattern tree --- */
  ITEM     cnt;                 /* number of items / heads */
  int      dir;                 /* processing direction */
  FIM16    *fim16;              /* 16-items machine */
  MEMSYS   *mem;                /* memory system for the nodes */
  FPNODE   root;                /* root node connecting trees */
  FPHEAD   heads[1];            /* header table (item lists) */
} FPTREE;                       /* (frequent pattern tree) */

typedef struct csnode {         /* --- children/sibling tree node --- */
  ITEM          id;             /* item/head identifier */
  SUPP          supp;           /* support (weight of transactions) */
  struct csnode *children;      /* list of child nodes */
  struct csnode *sibling;       /* successor node in sibling list */
  struct csnode *parent;        /* parent node (preceding item) */
  struct csnode *succ;          /* successor node with same item */
} CSNODE;                       /* (children/sibling tree node) */

typedef struct {                /* --- ch./sibling tree node list --- */
  ITEM     item;                /* associated item (item base code) */
  SUPP     supp;                /* support (weight of transactions) */
  CSNODE   *list;               /* list of nodes with this item */
} CSHEAD;                       /* (children/sibling tree head) */

typedef struct {                /* --- children/sibling tree --- */
  ITEM     cnt;                 /* number of items / heads */
  MEMSYS   *mem;                /* memory system for the nodes */
  CSNODE   root;                /* root node connecting trees */
  CSHEAD   heads[1];            /* header table (item lists) */
} CSTREE;                       /* (children/sibling tree) */

typedef struct tdnode {         /* --- top-down tree node --- */
  ITEM          id;             /* item/head identifier */
  SUPP          supp;           /* support (weight of transactions) */
  struct tdnode *children;      /* list of child nodes */
  struct tdnode *sibling;       /* successor node in sibling list */
} TDNODE;                       /* (top-down tree node) */

typedef struct {                /* --- top-down freq. pat. tree --- */
  ITEM     cnt;                 /* number of items / max. tree height */
  MEMSYS   *mem;                /* memory system for the nodes */
  TDNODE   *root;               /* root level of the tree */
  ITEM     items[1];            /* item identifier map */
} TDTREE;                       /* (top-down tree) */

typedef struct {                /* --- recursion data --- */
  int      mode;                /* mode flags (e.g. FPG_PERFECT) */
  int      dir;                 /* direction for item loops */
  SUPP     smin;                /* minimum support of an item set */
  ITEM     *set;                /* item set for projection */
  ITEM     *map;                /* item identifier map */
  SUPP     *cis;                /* conditional item support */
  FIM16    *fim16;              /* 16-items machine */
  ISREPORT *report;             /* item set reporter */
  ISTREE   *istree;             /* item set tree for fpg_tree() */
} RECDATA;                      /* (recursion data) */

typedef struct {                /* --- fpgrowth execution data --- */
  int      mode;                /* processing mode */
  ISTREE   *istree;             /* item set tree for rule generation */
} FPGROWTH;                     /* (fpgrowth execution data) */

typedef int FPGFN (TABAG *tabag, int target, SUPP smin, int mode,
                   ISREPORT *report);

/*----------------------------------------------------------------------
  Constants
----------------------------------------------------------------------*/
#if !defined QUIET && defined FPG_MAIN
/* --- error messages --- */
static const char *errmsgs[] = {
  /* E_NONE      0 */  "no error",
  /* E_NOMEM    -1 */  "not enough memory",
  /* E_FOPEN    -2 */  "cannot open file %s",
  /* E_FREAD    -3 */  "read error on file %s",
  /* E_FWRITE   -4 */  "write error on file %s",
  /* E_STDIN    -5 */  "double assignment of standard input",
  /* E_OPTION   -6 */  "unknown option -%c",
  /* E_OPTARG   -7 */  "missing option argument",
  /* E_ARGCNT   -8 */  "wrong number of arguments",
  /* E_TARGET   -9 */  "invalid target type '%c'",
  /* E_SIZE    -10 */  "invalid item set size %"ITEM_FMT,
  /* E_SUPPORT -11 */  "invalid minimum support %g",
  /* E_CONF    -12 */  "invalid minimum confidence %g",
  /* E_MEASURE -13 */  "invalid evaluation measure '%c'",
  /* E_AGGMODE -14 */  "invalid aggregation mode '%c'",
  /* E_NOITEMS -15 */  "no (frequent) items found",
  /* E_VARIANT -16 */  "invalid eclat variant '%c'",
  /*           -17 */  "unknown error"
};
#endif

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
#ifdef FPG_MAIN
#ifndef QUIET
static CCHAR    *prgname;       /* program name for error messages */
#endif
static TABREAD  *tread  = NULL; /* table/transaction reader */
static ITEMBASE *ibase  = NULL; /* item base */
static TABAG    *tabag  = NULL; /* transaction bag/multiset */
static ISREPORT *report = NULL; /* item set reporter */
static TABWRITE *twrite = NULL; /* table writer for pattern spectrum */
static double   *border = NULL; /* support border for filtering */
#endif

/*----------------------------------------------------------------------
  Auxiliary Functions for Debugging
----------------------------------------------------------------------*/
#ifndef NDEBUG

static void indent (int k)
{ while (--k >= 0) printf("   "); }

/*--------------------------------------------------------------------*/

static void fpt_show (FPTREE *fpt, ITEMBASE *base, int ind)
{                               /* --- show a freq. pattern tree */
  ITEM   i, k;                  /* loop variable */
  FPNODE *node;                 /* to traverse the node lists */

  assert(fpt);                  /* check the function arguments */
  printf("\n"); indent(ind); printf("------\n");
  indent(ind);                  /* indent the output line */
  printf("* (%"SUPP_FMT")\n", fpt->root.supp);
  for (i = 0; i < fpt->cnt; i++) {  /* traverse the items */
    indent(ind);                /* indent the output line */
    k = fpt->heads[i].item;     /* print item and support */
    if (k < 0) printf("%04x", (int)(k & ~TA_END));
    else       printf("%s/%"ITEM_FMT, ib_name(base, k), k);
    printf(":%"SUPP_FMT, fpt->heads[i].supp);
    for (node = fpt->heads[i].list; node; node = node->succ) {
      printf(" %"SUPP_FMT, node->supp); /* print the node support */
      k = node->parent->id;     /* get the parent item/list */
      if (k < 0) printf("[*]"); /* print a root node indicator */
      else { k = fpt->heads[k].item;
             printf("[%s/%"ITEM_FMT"]", ib_name(base, k), k); }
    }                           /* print the item in the parent */
    printf("\n");               /* terminate line after each node */
  }
  indent(ind); printf("------\n");
}  /* fpt_show() */

/*--------------------------------------------------------------------*/

static void cst_srec (CSNODE *node, int ind, CSHEAD *heads,
                      ITEMBASE *base)
{                               /* --- recursively show nodes */
  ITEM i;                       /* mapped item identifier */

  assert(ind >= 0);             /* check the function arguments */
  while (node) {                /* traverse the node list */
    indent(ind);                /* indent the output line */
    i = heads[node->id].item;   /* print the item name/bit pattern */
    if (i < 0) printf("%04x", (int)(i & ~TA_END));
    else       printf("%s/%"ITEM_FMT, ib_name(base, i), i);
    printf(":%"SUPP_FMT"\n",node->supp);/* print the node information */
    cst_srec(node->children, ind+1, heads, base);
    node = node->sibling;       /* recursively show the child nodes, */
  }                             /* then go to the next node */
}  /* cst_srec() */

/*--------------------------------------------------------------------*/

static void cst_show (CSTREE *cst, ITEMBASE *base, int ind)
{                               /* --- show a freq. pattern tree */
  assert(cst && (ind >= 0));    /* check the function arguments */
  printf("\n"); indent(ind); printf("------\n");
  indent(ind);                  /* indent the output line */
  printf("*:%"SUPP_FMT"\n", cst->root.supp);
  cst_srec(cst->root.children, ind+1, cst->heads, base);
  indent(ind); printf("------\n");
}  /* cst_show() */

/*--------------------------------------------------------------------*/

static void tdt_srec (TDNODE *node, int ind, const ITEM *items,
                      ITEMBASE *base)
{                               /* --- recursively show nodes */
  ITEM i;                       /* mapped item identifier */

  assert(ind >= 0);             /* check the function arguments */
  while (node) {                /* traverse the node list */
    indent(ind);                /* indent the output line */
    i = items[node->id];        /* print the item name/bit pattern */
    if (i < 0) printf("%04x", (int)(i & ~TA_END));
    else       printf("%s/%"ITEM_FMT, ib_name(base, i), i);
    printf(":%"SUPP_FMT"\n",node->supp);/* print the node information */
    tdt_srec(node->children, ind+1, items, base);
    node = node->sibling;       /* recursively show the child nodes, */
  }                             /* then go to the next node */
}  /* tdt_srec() */

/*--------------------------------------------------------------------*/

static void tdt_show (TDTREE *tdt, ITEMBASE *base, int ind)
{                               /* --- show a freq. pattern tree */
  assert(tdt && (ind >= 0));    /* check the function arguments */
  printf("\n"); indent(ind); printf("------\n");
  if (!tdt->root) { indent(ind); printf("(null)\n"); }
  else tdt_srec(tdt->root, ind, tdt->items, base);
  indent(ind); printf("------\n");  /* show the nodes recursively */
}  /* tdt_show() */

#endif  /* #ifndef NDEBUG */
/*----------------------------------------------------------------------
  Frequent Pattern Growth (simple nodes with only successor/parent)
----------------------------------------------------------------------*/

static int add_simple (FPTREE *fpt, const ITEM *ids, ITEM n, SUPP supp)
{                               /* --- add an item set to the tree */
  ITEM   i;                     /* buffer for an item */
  FPNODE *c, *node;             /* to create new (child) nodes */

  assert(fpt                    /* check the function arguments */
  &&    (ids || (n <= 0)) && (supp >= 0));
  node = &fpt->root;            /* start at the root node and */
  while (1) {                   /* traverse the items of the set */
    node->supp += supp;         /* update the item set support */
    if (--n < 0) return 0;      /* if all items are processed, abort */
    i = *ids++;                 /* get the next item in the set */
    c = fpt->heads[i].list;     /* and the corresponding list head */
    if (!c || (c->parent != node)) break;
    node = c;                   /* if last set is not matched, abort, */
  }                             /* otherwise go to the existing node */
  while (1) {                   /* traverse the new items */
    c = (FPNODE*)ms_alloc(fpt->mem);
    if (!c) return -1;          /* allocate a new tree node */
    c->id     = i;              /* store the next item/list and */
    c->supp   = supp;           /* the support of the item set */
    c->parent = node;           /* connect to the parent node */
    c->succ   = fpt->heads[i].list;   /* add the new node */
    fpt->heads[i].list = node = c;    /* to the item list */
    if (--n < 0) return 1;      /* if there are no more items, */
    i = *ids++;                 /* abort the insertion loop, */
  }                             /* otherwise get the next item */
}  /* add_simple() */

/*--------------------------------------------------------------------*/

static int proj_simple (FPTREE *dst, FPTREE *src, ITEM id, RECDATA *rd)
{                               /* --- project a freq. pattern tree */
  ITEM   i, n;                  /* loop variables, item buffer */
  SUPP   pex;                   /* minimum support for perf. exts. */
  SUPP   *s;                    /* to sum the support values */
  ITEM   *map, *d;              /* to build the item map */
  FPHEAD *h;                    /* to access the node headers */
  FPNODE *node, *anc;           /* to traverse the tree nodes */

  assert(dst                    /* check the function arguments */
  &&     src && (id >= 0) && rd);
  memset(s = rd->cis, 0, (size_t)id *sizeof(SUPP));
  for (node = src->heads[id].list; node; node = node->succ)
    for (anc = node->parent; anc->id >= 0; anc = anc->parent)
      s[anc->id] += node->supp; /* compute the conditional support */
  /* Using a two-dimensional table that is filled when a frequent */
  /* pattern tree is created proved to be slower than the above.  */
  pex = (rd->mode & FPG_PERFECT) ? src->heads[id].supp : SUPP_MAX;
  map = rd->map;                /* get perfect extension support */
  for (i = n = 0; i < id; i++){ /* traverse the items that */
    if (s[i] <  rd->smin) {     /* precede the projection item, */
      map[i] = -1; continue; }  /* eliminate infrequent items and */
    if (s[i] >= pex) {          /* collect perfect extension items */
      map[i] = -1; isr_addpex(rd->report,src->heads[i].item); continue;}
    map[i] = n;                 /* build the item identifier map */
    h = dst->heads +n++;        /* init. the destination header */
    h->item = src->heads[i].item;
    h->supp = s[i];             /* note the conditional item support */
    h->list = NULL;             /* and clear the node list */
  }
  if (n <= 0) return 0;         /* if the projection is empty, abort */
  dst->cnt       = n;           /* note the number of items and */
  dst->root.supp = 0;           /* init. root node and node heads */
  for (node = src->heads[id].list; node; node = node->succ) {
    d = map;                    /* traverse the item list */
    for (anc = node->parent; anc->id > TA_END; anc = anc->parent)
      if ((i = map[anc->id]) >= 0) *--d = i;
    if (add_simple(dst, d, (ITEM)(map-d), node->supp) < 0)
      return -1;                /* collect the non-eliminated items */
  }                             /* and add reduced trans. to the tree */
  return 1;                     /* return that result is not empty */
}  /* proj_simple() */

/*--------------------------------------------------------------------*/

static int rec_simple (FPTREE *fpt, RECDATA *rd)
{                               /* --- find item sets recursively */
  int    r;                     /* error status */
  ITEM   i, z;                  /* loop variables */
  FPHEAD *h;                    /* node list for current item */
  FPNODE *node, *anc;           /* to traverse the tree nodes */
  FPTREE *proj = NULL;          /* projected frequent pattern tree */
  ITEM   *s;                    /* to collect the tail items */

  assert(fpt && rd);            /* check the function arguments */
  #ifdef FPG_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & FPG_TAIL) {    /* if to use head union tail pruning */
    for (s = rd->set, i = 0; i < fpt->cnt; i++)
      s[i] = fpt->heads[i].item;/* collect the tail items */
    r = isr_tail(rd->report, s, i);
    if (r) return r;            /* if tail needs no processing, */
  }                             /* abort the recursion */
  if ((fpt->cnt > 1)            /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (FPTREE*)malloc(sizeof(FPTREE)
                         +(size_t)(fpt->cnt-2) *sizeof(FPHEAD));
    if (!proj) return -1;       /* create a frequent pattern tree */
    proj->root.id   = TA_END;   /* of the maximally possible size */
    proj->root.succ = proj->root.parent = NULL;
    proj->dir = fpt->dir;       /* initialize the root node and */
    proj->mem = fpt->mem;       /* copy the processing direction */
    if (ms_push(fpt->mem) < 0) { free(proj); return -1; }
  }                             /* note the current memory state */
  if (fpt->dir > 0) { z = fpt->cnt; i = 0; }
  else              { z = -1;       i = fpt->cnt-1; }
  for (r = 0; i != z; i += fpt->dir) {
    h = fpt->heads +i;          /* traverse the frequent items */
    r = isr_add(rd->report, h->item, h->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    node = h->list;             /* get the head of the item list */
    if (!node->succ) {          /* if projection would be a chain */
      for (anc = node->parent; anc->id > TA_END; anc = anc->parent) {
        isr_addpex(rd->report, fpt->heads[anc->id].item);
      } }                       /* add items as perfect extensions */
    else if (proj) {            /* if another item can be added */
      r = proj_simple(proj, fpt, i, rd);
      if (r > 0) r = rec_simple(proj, rd);
      if (r < 0) break;         /* project frequent pattern tree and */
    }                           /* find freq. item sets recursively */
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) {                   /* delete the created projection */
    free(proj); ms_pop(fpt->mem); }
  return r;                     /* return the error status */
}  /* rec_simple() */

/*--------------------------------------------------------------------*/

static int add_smp16 (FPTREE *fpt, const ITEM *ids, ITEM n, SUPP supp)
{                               /* --- add an item set to the tree */
  ITEM   i;                     /* buffer for an item */
  FPNODE *c, *node;             /* to create new (child) nodes */

  assert(fpt                    /* check the function arguments */
  &&    (ids || (n <= 0)) && (supp >= 0));
  node = &fpt->root;            /* start at the root node and */
  node->supp += supp;           /* update the item set support */
  if (--n < 0) return 0;        /* if there are no items, abort */
  if (*ids < 0) {               /* if there are packed items, */
    i = *ids++;                 /* get and skip the packed items and */
    if (fpt->dir > 0)           /* add them to the 16-items machine */
      m16_add(fpt->fim16, (BITTA)(i & ~TA_END), supp);
    node = fpt->heads[0].list;  /* get the corresponding list head */
    if (node && (node->id == i))
      node->supp += supp;       /* if node exists, add the support */
    else {                      /* if no proper node exists */
      c = (FPNODE*)ms_alloc(fpt->mem);
      if (!c) return -1;        /* allocate a new tree node */
      c->id     = i;            /* store the packed items and */
      c->supp   = supp;         /* the support of the item set */
      c->parent = &fpt->root;   /* connect to the root node */
      c->succ   = node;         /* add the new node to the list */
      fpt->heads[0].list = node = c;    /* for the packed items */
    }                           /* (all packed items are in list 0) */
    if (--n < 0) return 0;      /* if there are no other items, abort */
  }
  while (1) {                   /* traverse the items of the set */
    i = *ids++;                 /* get the next item in the set */
    c = fpt->heads[i].list;     /* and the corresponding list head */
    if (!c || (c->parent != node)) break;
    node = c;                   /* if last set is not matched, abort, */
    node->supp += supp;         /* otherwise go to the existing node, */
    if (--n < 0) return 0;      /* update the item set support, and */
  }                             /* if all items are processed, abort */
  while (1) {                   /* traverse the new items */
    c = (FPNODE*)ms_alloc(fpt->mem);
    if (!c) return -1;          /* allocate a new tree node */
    c->id     = i;              /* store the next item and */
    c->supp   = supp;           /* the support of the item set */
    c->parent = node;           /* connect to the parent node */
    c->succ   = fpt->heads[i].list;   /* add the new node */
    fpt->heads[i].list = node = c;    /* to the item list */
    if (--n < 0) return 1;      /* if there are no more items, */
    i = *ids++;                 /* abort the insertion loop, */
  }                             /* otherwise get the next item */
}  /* add_smp16() */

/*--------------------------------------------------------------------*/

static int proj_smp16 (FPTREE *dst, FPTREE *src, ITEM id,
                       ITEM mask, RECDATA *rd)
{                               /* --- project a freq. pattern tree */
  ITEM   i, n;                  /* loop variables, item buffer */
  SUPP   pex;                   /* minimum support for perf. exts. */
  SUPP   *s;                    /* to sum the support values */
  ITEM   *map, *d;              /* to build the item map */
  FPHEAD *h;                    /* to access the node heads */
  FPNODE *node, *anc;           /* to traverse the tree nodes */

  assert(dst                    /* check the function arguments */
  &&     src && (id >= 0) && rd);
  memset(s = rd->cis, 0, (size_t)id *sizeof(SUPP));
  for (node = src->heads[id].list; node; node = node->succ)
    for (anc = node->parent; anc->id >= 0; anc = anc->parent)
      s[anc->id] += node->supp; /* compute the conditional support */
  /* Using a two-dimensional table that is filled when a frequent */
  /* pattern tree is created proved to be slower than the above.  */
  pex = (rd->mode & FPG_PERFECT) ? src->heads[id].supp : SUPP_MAX;
  map = rd->map;                /* get perfect extension support */
  h   = dst->heads;             /* always keep head for packed items */
  h->item = -1; h->supp = 0; h->list = NULL;
  for (i = n = 1; i < id; i++){ /* traverse the items that */
    if (s[i] <  rd->smin) {     /* precede the projection item, */
      map[i] = -1; continue; }  /* eliminate infrequent items and */
    if (s[i] >= pex) {          /* collect perfect extension items */
      map[i] = -1; isr_addpex(rd->report,src->heads[i].item); continue;}
    map[i] = n;                 /* build the item identifier map */
    h = dst->heads +n++;        /* init. the destination header */
    h->item = src->heads[i].item;
    h->supp = s[i];             /* note the conditional item support */
    h->list = NULL;             /* and clear the node list */
  }
  if (n <= 0) return 0;         /* if the projection is empty, abort */
  dst->cnt       = n;           /* note the number of items and */
  dst->root.supp = 0;           /* init. root node and node heads */
  for (node = src->heads[id].list; node; node = node->succ) {
    d = map;                    /* traverse the item list */
    for (anc = node->parent; anc->id > TA_END; anc = anc->parent) {
      i = anc->id;              /* traverse path to the root */
      if (i < 0) { if ((i &= mask)) *--d = i | TA_END; }
      else if ((i = map[i]) >= 0)   *--d = i;
    }                           /* collect the non-eliminated items */
    if (add_smp16(dst, d, (ITEM)(map-d), node->supp) < 0)
      return -1;                /* add reduced trans. to the tree */
  }
  return 1;                     /* return that result is not empty */
}  /* proj_smp16() */

/*--------------------------------------------------------------------*/

static int rec_smp16 (FPTREE *fpt, RECDATA *rd)
{                               /* --- find item sets recursively */
  int    r;                     /* error status */
  ITEM   i, k, z;               /* loop variables */
  ITEM   mask;                  /* mask for packed items to keep */
  FPHEAD *h;                    /* node list for current item */
  FPNODE *node, *anc;           /* to traverse the tree nodes */
  FPTREE *proj = NULL;          /* projected frequent pattern tree */
  ITEM   *s;                    /* to collect the tail items */

  assert(fpt && rd);            /* check the function arguments */
  #ifdef FPG_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & FPG_TAIL) {    /* if to use head union tail pruning */
    for (s = rd->set, i = 0; i < fpt->cnt; i++)
      s[i] = fpt->heads[i].item;/* collect the tail items */
    r = isr_tail(rd->report, s, i);
    if (r) return r;            /* if tail needs no processing, */
  }                             /* abort the recursion */
  if ((fpt->cnt > 1)            /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (FPTREE*)malloc(sizeof(FPTREE)
                         +(size_t)(fpt->cnt-2) *sizeof(FPHEAD));
    if (!proj) return -1;       /* create a frequent pattern tree */
    proj->root.id   = TA_END;   /* of the maximally possible size */
    proj->root.succ = proj->root.parent = NULL;
    proj->dir   = fpt->dir;     /* initialize the root node and */
    proj->fim16 = fpt->fim16;   /* copy the processing direction */
    proj->mem   = fpt->mem;     /* and the 16-items machine */
    if (ms_push(fpt->mem) < 0) { free(proj); return -1; }
  }                             /* note the current memory state */
  mask = ITEM_MAX;              /* init. the packed item mask */
  if (fpt->dir > 0) { z = fpt->cnt; i = 0; }
  else              { z = -1;       i = fpt->cnt-1; }
  for (r = 0; i != z; i += fpt->dir) {
    h = fpt->heads +i;          /* traverse the frequent items */
    if (h->item < 0) {          /* if to use a 16-items machine */
      if (fpt->dir < 0)         /* if downward processing direction */
        for (node = h->list; node; node = node->succ)
          m16_add(fpt->fim16, (BITTA)(node->id & ~TA_END), node->supp);
      r = m16_mine(fpt->fim16); /* add bit-rep. transaction prefixes */
      if (r < 0) break;         /* to the 16-items machine and mine */
      mask = r; continue;       /* get the packed items mask */
    }                           /* and go to the next item list */
    r = isr_add(rd->report, h->item, h->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    node = h->list;             /* get the head of the item list */
    if (!node->succ) {          /* if projection would be a chain */
      for (anc = node->parent; anc->id > TA_END; anc = anc->parent) {
        k = anc->id;            /* traverse the list of ancestors */
        if (k >= 0) isr_addpex  (rd->report, fpt->heads[k].item);
        else        isr_addpexpk(rd->report, k);
      } }                       /* add items as perfect extensions */
    else if (proj) {            /* if another item can be added */
      r = proj_smp16(proj, fpt, i, mask, rd);
      if (r > 0) r = rec_smp16(proj, rd);
      if (r < 0) break;         /* project frequent pattern tree and */
    }                           /* find freq. item sets recursively */
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) {                   /* delete the created projection */
    free(proj); ms_pop(fpt->mem); }
  return r;                     /* return the error status */
}  /* rec_smp16() */

/*--------------------------------------------------------------------*/

int fpg_simple (TABAG *tabag, int target, SUPP smin, int mode,
                ISREPORT *report)
{                               /* --- search for frequent item sets */
  int        r = 0;             /* result of recursion/functions */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        j, n;              /* loop variable, number of trans. */
  SUPP       pex;               /* minimum support for perf. exts. */
  TRACT      *t;                /* to traverse the transactions */
  ITEM       *s, *d;            /* to build the item maps */
  const ITEM *p;                /* to traverse transaction items */
  const SUPP *f;                /* item frequencies in trans. bag */
  FPTREE     *fpt;              /* created frequent pattern tree */
  FPHEAD     *h;                /* to traverse the item heads */
  RECDATA    rd;                /* structure for recursive search */

  assert(tabag && report);      /* check the function arguments */
  rd.mode = mode;               /* store search mode and item dir. */
  rd.dir  = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin = (smin > 0) ? smin : 1;    /* check and adapt the support */
  pex     = tbg_wgt(tabag);     /* check against the minimum support */
  if (rd.smin > pex) return 0;  /* and get minimum for perfect exts. */
  if (!(mode & FPG_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  f = tbg_ifrqs(tabag, 0);      /* get the item frequencies */
  if (!f) return -1;            /* in the transaction bag */
  s = rd.set = (ITEM*)malloc((size_t)(k+k) *sizeof(ITEM)
                            +(size_t) k    *sizeof(SUPP));
  if (!s) return -1;            /* create item and support arrays */
  rd.map = d = s+k;             /* note item map and set buffer */
  rd.cis = (SUPP*)(d+k);        /* and the item support array */
  if (!(mode & FPG_FIM16)) m = 0;   /* if to use a 16-items machine, */
  else { d[0] = s[0] = 0;  m = 1; } /* always keep the packed items */
  for (i = m; i < k; i++) {     /* build the item identifier map */
    if (f[i] <  rd.smin) { d[i] = -1;                        continue; }
    if (f[i] >= pex)     { d[i] = -1; isr_addpex(report, i); continue; }
    d[i] = m; s[m++] = i;       /* eliminate infrequent items and */
  }                             /* collect perfect extension items */
  if (m <= 0) {                 /* check whether there are items left */
    r = isr_report(report); free(rd.set); return r; }
  fpt = (FPTREE*)malloc(sizeof(FPTREE) +(size_t)(m-1) *sizeof(FPHEAD));
  if (!fpt) { free(rd.set); return -1; }
  fpt->cnt = k = m;             /* allocate the base tree structure */
  fpt->dir = rd.dir;            /* and initialize its fields */
  fpt->mem = ms_create(sizeof(FPNODE), 65535);
  if (!fpt->mem) { free(fpt); free(rd.set); return -1; }
  fpt->root.id   = TA_END;      /* create memory system for the nodes */
  fpt->root.supp = 0;           /* and initialize the root node */
  fpt->root.succ = fpt->root.parent = NULL;
  for (i = 0; i < k; i++) {     /* initialize the header table */
    h = fpt->heads+i; h->supp = f[h->item = s[i]]; h->list = NULL; }
  if (mode & FPG_FIM16) {       /* if to use a 16-items machine */
    fpt->fim16 = m16_create(rd.dir, rd.smin, report);
    if (!fpt->fim16) { ms_delete(fpt->mem);
      free(fpt); free(rd.set); return -1; }
    fpt->heads[0].item = -1;    /* create a 16-items machine */
    for (j = n; --j >= 0; ) {   /* traverse the transactions and */
      t = tbg_tract(tabag, j);  /* collect the non-eliminated items */
      for (k = 0, p = ta_items(t); *p > TA_END; p++) {
        if      ((m = *p)   <  0) s[k++] = m;
        else if ((m = d[m]) >= 0) s[k++] = m;
      }                         /* (packed items are always copied) */
      r = add_smp16(fpt, s, k, ta_wgt(t));
      if (r < 0) break;         /* add the reduced transaction */
    }                           /* to the frequent pattern tree */
    if (r >= 0) {               /* if a frequent pattern tree */
      rd.report = report;       /* has successfully been built, */
      r = rec_smp16(fpt, &rd);  /* find freq. item sets recursively */
      if (r >= 0) r = isr_report(report);
    }                           /* report the empty item set */
    m16_delete(fpt->fim16); }   /* delete the 16-items machine */
  else {                        /* if not to use a 16-items machine */
    for (j = n; --j >= 0; ) {   /* traverse the transactions and */
      t = tbg_tract(tabag, j);  /* collect the non-eliminated items */
      for (k = 0, p = ta_items(t); *p > TA_END; p++)
        if ((m = d[*p]) >= 0) s[k++] = m;
      r = add_simple(fpt, s, k, ta_wgt(t));
      if (r < 0) break;         /* add the reduced transaction */
    }                           /* to the frequent pattern tree */
    if (r >= 0) {               /* if a frequent pattern tree */
      rd.report = report;       /* has successfully been built, */
      r = rec_simple(fpt, &rd); /* find freq. item sets recursively */
      if (r >= 0) r = isr_report(report);
    }                           /* report the empty item set */
  }
  ms_delete(fpt->mem);          /* delete the memory mgmt. system */
  free(fpt); free(rd.set);      /* and the frequent pattern tree */
  return r;                     /* return the error status */
}  /* fpg_simple() */

/*----------------------------------------------------------------------
  Frequent Pattern Growth (complex nodes with children/sibling)
----------------------------------------------------------------------*/

static int add_cmplx (CSTREE *cst, const ITEM *ids, ITEM n, SUPP supp)
{                               /* --- add an item set to the tree */
  ITEM   i;                     /* buffer for an item */
  CSNODE *node;                 /* to traverse the nodes */
  CSNODE **p, *c;               /* to create new nodes */

  assert(cst                    /* check the function arguments */
  &&    (ids || (n <= 0)) && (supp >= 0));
  node = &cst->root;            /* start at the root node and */
  while (1) {                   /* traverse the items of the set */
    node->supp += supp;         /* update the item set support */
    if (--n < 0) return 0;      /* if all items are processed, abort */
    i = *ids++;                 /* get the next item in the set and */
    p = &node->children;        /* traverse the list of children */
    while (*p && ((*p)->id < i)) p = &(*p)->sibling;
    if (!(c = *p) || (c->id != i)) break;
    node = c;                   /* find the item/insertion position */
  }                             /* and if item does not exist, abort */
  c = (CSNODE*)ms_alloc(cst->mem);
  if (!c) return -1;            /* create a new prefix tree node */
  c->id      = i;               /* store the current item and */
  c->supp    = supp;            /* the support of the item set */
  c->parent  = node;            /* connect to the parent node */
  c->sibling = *p;              /* insert the created node into */
  c->succ    = cst->heads[i].list;      /* the sibling list and */
  cst->heads[i].list = node = *p = c;   /* into the item list */
  while (--n >= 0) {            /* traverse the rest of the items */
    node->children = c = (CSNODE*)ms_alloc(cst->mem);
    if (!c) return -1;          /* create a new prefix tree node */
    c->id  = i = *ids++;        /* store the next item and */
    c->supp    = supp;          /* the support of the item set */
    c->parent  = node;          /* connect to the parent node */
    c->sibling = NULL;          /* there are no siblings yet */
    c->succ    = cst->heads[i].list;
    cst->heads[i].list = node = c;
  }                             /* insert node into the item list */
  node->children = NULL;        /* last created node is a leaf */
  return 1;                     /* return that nodes were added */
}  /* add_cmplx() */

/*--------------------------------------------------------------------*/

static int proj_cmplx (CSTREE *dst, CSTREE *src, ITEM id, RECDATA *rd)
{                               /* --- project a freq. pattern tree */
  int    r;                     /* result of function call */
  ITEM   i, n;                  /* loop variables, identifier buffer */
  SUPP   pex;                   /* minimum support for perfect exts. */
  SUPP   *s;                    /* to sum the support values */
  ITEM   *map, *d;              /* to build the item map */
  CSHEAD *h;                    /* to traverse the item heads */
  CSNODE *node, *anc;           /* to traverse the tree nodes */

  assert(dst                    /* check the function arguments */
  &&     src && (id >= 0) && rd);
  memset(s = rd->cis, 0, (size_t)id *sizeof(SUPP));
  for (node = src->heads[id].list; node; node = node->succ)
    for (anc = node->parent; anc->id >= 0; anc = anc->parent)
      s[anc->id] += node->supp; /* compute the conditional support */
  /* Using a two-dimensional table that is filled when a frequent */
  /* pattern tree is created proved to be slower than the above.  */
  pex = (rd->mode & FPG_PERFECT) ? src->heads[id].supp : SUPP_MAX;
  map = rd->map;                /* get perfect extension support */
  for (i = n = 0; i < id; i++){ /* traverse the items that */
    if (s[i] <  rd->smin) {     /* precede the projection item, */
      map[i] = -1; continue; }  /* eliminate infrequent items and */
    if (s[i] >= pex) {          /* collect perfect extension items */
      map[i] = -1; isr_addpex(rd->report,src->heads[i].item); continue;}
    map[i] = n;                 /* build the item identifier map */
    h = dst->heads +n++;        /* init. the destination item list */
    h->item = src->heads[i].item;
    h->supp = s[i];             /* note the conditional item support */
    h->list = NULL;             /* and clear the node list */
  }
  if (n <= 0) return 0;         /* if the projection is empty, abort */
  dst->cnt = n;                 /* note the number of items */
  if ((n <= 16) && rd->fim16) { /* if at most 16 items left, */
    h = dst->heads;             /* traverse the remaining items */
    for (i = 0; i < n; i++)     /* and set the item identifier map */
      m16_setmap(rd->fim16, i, h[i].item);
    for (node = src->heads[id].list; node; node = node->succ) {
      n = 0;                    /* traverse the item list */
      for (anc = node->parent; anc->id >= 0; anc = anc->parent)
        if ((i = map[anc->id]) >= 0) n |= 1 << i;
      m16_add(rd->fim16, (BITTA)(n & ~TA_END), node->supp);
    }                           /* add bit-represented transactions */
    r = m16_mine(rd->fim16);    /* to the 16-items machine and mine */
    return (r < 0) ? r : 0;     /* return 'no projection created' */
  }
  dst->root.supp     = 0;       /* init. the root node support */
  dst->root.children = NULL;    /* and clear the child list */
  #if 0
  if (rd->fim16                 /* if to use a 16-items machine */
  && (rd->dir > 0)) {           /* and forward processing direction */
    for (node = src->heads[id].list; node; node = node->succ) {
      d = map; n = 0;           /* traverse the item list */
      for (anc = node->parent; anc->id >= 0; anc = anc->parent)
        if ((i = map[anc->id]) >= 0) {
          *--d = i; if (i < 16) n |= 1 << i; }
      m16_add(rd->fim16, (BITTA)(n & ~TA_END), node->supp);
      if (add_cmplx(dst, d, (ITEM)(map-d), node->supp) < 0)
        return -1;              /* collect the non-eliminated items */
    }                           /* and add reduced trans. to the tree */
    for (i = 0; i < 16; i++) {  /* traverse the first 16 items and */
      h = dst->heads +i; h->supp = 0;       /* clear their support */
      m16_setmap(rd->fim16, i, h->item);
    }                           /* set the item identifier map */
    r = m16_mine(rd->fim16);    /* mine with 16-items machine */
    if (r < 0) return r; }      /* and check for an error */
  else {                        /* if not to use a 16-items machine */
    for (node = src->heads[id].list; node; node = node->succ) {
      d = s;                    /* traverse the item list */
      for (anc = node->parent; anc->id >= 0; anc = anc->parent)
        if ((i = map[anc->id]) >= 0) *--d = i;
      if (add_cmplx(dst, d, (ITEM)(map-d), node->supp) < 0)
        return -1;              /* collect the non-eliminated items */
    }                           /* and add the reduced transactions */
  }                             /* to the destination tree */
  /* Trying to process the lowest 16 items with a 16-items machine */
  /* in this way is no faster, because the fp-tree has to be built */
  /* fully in any case, so that projections can be computed.       */
  #else
  for (node = src->heads[id].list; node; node = node->succ) {
    d = map;                    /* traverse the item list */
    for (anc = node->parent; anc->id >= 0; anc = anc->parent)
      if ((i = map[anc->id]) >= 0) *--d = i;
    if (add_cmplx(dst, d, (ITEM)(map-d), node->supp) < 0)
      return -1;                /* collect the non-eliminated items */
  }                             /* and add reduced trans. to the tree */
  #endif
  return 1;                     /* return 'projection created' */
}  /* proj_cmplx() */

/*--------------------------------------------------------------------*/

static int add_reord (CSTREE *cst, ITEM *flags, ITEM n, SUPP supp)
{                               /* --- add an item set to the tree */
  ITEM   i;                     /* buffer for an item */
  CSNODE *node;                 /* to traverse the nodes */
  CSNODE **p, *c;               /* to create new nodes */

  assert(cst                    /* check the function arguments */
  &&    (n >= 0) && (supp >= 0));
  node = &cst->root;            /* start at the root node and */
  node->supp += supp;           /* update the empty set support */
  if (n <= 0) return 0;         /* if transaction is empty, abort */
  for (i = 0; 1; i++) {         /* traverse the items in the tree */
    if (!flags[i]) continue;    /* if item is not in set, skip it */
    flags[i] = 0;               /* clear the containment flag */
    p = &node->children;        /* traverse the list of children */
    while (*p && ((*p)->id < i)) p = &(*p)->sibling;
    if (!(c = *p) || (c->id != i)) break;
    node = c;                   /* find the item/insertion position */
    node->supp += supp;         /* if the item does not exist, abort */
    if (--n <= 0) return 0;     /* otherwise update the support */
  }                             /* and check for last item */
  c = (CSNODE*)ms_alloc(cst->mem);
  if (!c) return -1;            /* create a new prefix tree node */
  c->id      = i;               /* store the current item and */
  c->supp    = supp;            /* the support of the item set */
  c->parent  = node;            /* connect to the parent node */
  c->sibling = *p;              /* insert the created node into */
  c->succ    = cst->heads[i].list;      /* the sibling list and */
  cst->heads[i].list = node = *p = c;   /* into the item list */
  if (--n <= 0) { node->children = NULL; return 1; }
  while (1) {                   /* traverse the remaining items */
    if (!flags[++i]) continue;  /* if item is not in set, skip it */
    flags[i] = 0;               /* clear the containment flag */
    node->children = c = (CSNODE*)ms_alloc(cst->mem);
    if (!c) return -1;          /* create a new prefix tree node */
    c->id      = i;             /* store the next item and */
    c->supp    = supp;          /* the support of the item set */
    c->parent  = node;          /* connect to the parent node */
    c->sibling = NULL;          /* there are no siblings yet */
    c->succ    = cst->heads[i].list;  /* insert the new node */
    cst->heads[i].list = node = c;    /* into the item list */
    if (--n <= 0) break;        /* check for last item */
  }
  node->children = NULL;        /* last created node is a leaf */
  return 1;                     /* return that nodes were added */
}  /* add_reord() */

/*--------------------------------------------------------------------*/

static int proj_reord (CSTREE *dst, CSTREE *src, ITEM id, RECDATA *rd)
{                               /* --- project a freq. pattern tree */
  int    r;                     /* result of function calls */
  ITEM   i, k, n;               /* loop variables, item buffer */
  SUPP   pex;                   /* minimum support for perfect exts. */
  SUPP   *s;                    /* to sum the support values */
  ITEM   *map, *d;              /* to build the item map */
  CSHEAD *h;                    /* to traverse the item heads */
  CSNODE *node, *anc;           /* to traverse the tree nodes */

  assert(dst                    /* check the function arguments */
  &&     src && (id >= 0) && rd);
  memset(s = rd->cis, 0, (size_t)id *sizeof(SUPP));
  for (node = src->heads[id].list; node; node = node->succ)
    for (anc = node->parent; anc->id >= 0; anc = anc->parent)
      s[anc->id] += node->supp; /* compute the conditional support */
  /* Using a two-dimensional table that is filled when a frequent */
  /* pattern tree is created proved to be slower than the above.  */
  pex = (rd->mode & FPG_PERFECT) ? src->heads[id].supp : SUPP_MAX;
  map = rd->set;                /* get perfect extension support */
  for (d = map +id, i = n = 0; i < id; i++) {
    if (s[i] <  rd->smin) {     /* traverse items and their support */
      map[i] = -1; continue; }  /* eliminate infrequent items and */
    if (s[i] >= pex) {          /* collect perfect extension items */
      map[i] = -1; isr_addpex(rd->report,src->heads[i].item); continue;}
    d[n++] = i;                 /* collect the remaining items */
  }                             /* in a temporary buffer */
  if (n <= 0) return 0;         /* if the projection is empty, abort */
  dst->cnt = n;                 /* note the number of items */
  i2s_sort(d, (size_t)n,-1, s); /* sort the rem. items descendingly */
  for (i = 0; i < n; i++) {     /* traverse the sorted items */
    h = dst->heads +i;          /* init. the destination item list */
    h->item = src->heads[k = d[i]].item;
    h->supp = s[k]; map[k] = i; /* build the item identifier maps */
    h->list = NULL;             /* and store the item support */
  }
  if ((n <= 16) && rd->fim16) { /* if at most 16-items left */
    h = dst->heads;             /* traverse the remaining items */
    for (i = 0; i < n; i++)     /* and set the item identifier map */
      m16_setmap(rd->fim16, i, h[i].item);
    for (node = src->heads[id].list; node; node = node->succ) {
      n = 0;                    /* traverse the item list */
      for (anc = node->parent; anc->id >= 0; anc = anc->parent)
        if ((i = map[anc->id]) >= 0) n |= 1 << i;
      m16_add(rd->fim16, (BITTA)(n & ~TA_END), node->supp);
    }                           /* add bit-represented transactions */
    r = m16_mine(rd->fim16);    /* to the 16-items machine and mine */
    return (r < 0) ? r : 0;     /* return 'no projection created' */
  }
  dst->root.supp     = 0;       /* init. the root node support */
  dst->root.children = NULL;    /* and clear the child list */
  #if 0
  memset(d, 0, (size_t)id *sizeof(ITEM));
  if (rd->fim16                 /* if to use a 16-items machine */
  && (rd->dir > 0)) {           /* and forward processing direction */
    for (node = src->heads[id].list; node; node = node->succ) {
      n = k = 0;                /* traverse the item list */
      for (anc = node->parent; anc->id >= 0; anc = anc->parent)
        if ((i = map[anc->id]) >= 0) {
          k += d[i] = 1; if (i < 16) n |= 1 << i; }
      m16_add(rd->fim16, (BITTA)(n & ~TA_END), node->supp);
      if (add_reord(dst, d, k, node->supp) < 0)
        return -1;              /* collect the non-eliminated items */
    }                           /* and add reduced trans. to the tree */
    for (i = 0; i < 16; i++) {  /* traverse the first 16 items and */
      h = dst->heads +i; h->supp = 0;       /* clear their support */
      m16_setmap(rd->fim16, i, h->item);
    }                           /* set the item identifier map */
    r = m16_mine(rd->fim16);    /* mine with 16-items machine */
    if (r < 0) return r; }      /* and check for an error */
  else {                        /* if not to use a 16-items machine */
    for (node = src->heads[id].list; node; node = node->succ) {
      k = 0;                    /* traverse the item list */
      for (anc = node->parent; anc->id >= 0; anc = anc->parent)
        if ((i = map[anc->id]) >= 0) k += d[i] = 1;
      if (add_reord(dst, d, k, node->supp) < 0)
        return -1;              /* collect the non-eliminated items */
    }                           /* and add the reduced transactions */
  }                             /* to the destination tree */
  /* Trying to process the lowest 16 items with a 16-items machine */
  /* in this way is no faster, because the fp-tree has to be built */
  /* fully in any case, since its projections need to be computed. */
  #elif 1                       /* this version is slightly faster */
  memset(d, 0, (size_t)id *sizeof(ITEM));
  for (node = src->heads[id].list; node; node = node->succ) {
    k = 0;                      /* traverse the item list */
    for (anc = node->parent; anc->id >= 0; anc = anc->parent)
      if ((i = map[anc->id]) >= 0) k += d[i] = 1;
    if (add_reord(dst, d, k, node->supp) < 0)
      return -1;                /* collect the non-eliminated items */
  }                             /* and add reduced trans. to the tree */
  #else                         /* this version is slightly slower */
  for (node = src->heads[id].list; node; node = node->succ) {
    d = map;                    /* traverse the item list */
    for (anc = node->parent; anc->id >= 0; anc = anc->parent)
      if ((i = map[anc->id]) >= 0) *--d = i;
    ia_qsort(d, (size_t)(k = (ITEM)(map-d)), +1);
    if (add_cmplx(dst, d, k, node->supp) < 0)
      return -1;                /* sort the collected items and */
  }                             /* add the red. trans. to the tree */
  #endif
  return 1;                     /* return 'projection created' */
}  /* proj_reord() */

/*--------------------------------------------------------------------*/

static int rec_cmplx (CSTREE *cst, RECDATA *rd)
{                               /* --- find item sets recursively */
  int    r;                     /* error status */
  ITEM   i, z;                  /* loop variables */
  CSHEAD *h;                    /* node list for current item */
  CSNODE *node;                 /* to traverse the tree nodes */
  CSTREE *proj = NULL;          /* projected frequent pattern tree */
  ITEM   *s;                    /* to collect the tail items */

  assert(cst && rd);            /* check the function arguments */
  #ifdef FPG_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & FPG_TAIL) {    /* if to use head union tail pruning */
    for (s = rd->set, i = 0; i < cst->cnt; i++)
      s[i] = cst->heads[i].item;/* collect the tail items */
    r = isr_tail(rd->report, s, i);
    if (r) return r;            /* if tail needs no processing, */
  }                             /* abort the recursion */
  if ((cst->cnt > 1)            /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (CSTREE*)malloc(sizeof(CSTREE)
                         +(size_t)(cst->cnt-2) *sizeof(CSHEAD));
    if (!proj) return -1;       /* create a frequent pattern tree */
    proj->root.id   = TA_END;   /* of the maximally possible size */
    proj->root.succ = proj->root.parent = proj->root.sibling = NULL;
    proj->mem = cst->mem;       /* initialize the root node */
    if (ms_push(cst->mem) < 0) { free(proj); return -1; }
  }                             /* note the current memory state */
  #if 0                         /* needed for 16-items machine */
  i = (cst->heads[0].supp <= 0) ? 16 : 0;
  if (rd->dir > 0) { z = cst->cnt; }
  else             { z = i-1; i = cst->cnt-1; }
  #else                         /* if no 16-items machine is used */
  if (rd->dir > 0) { z = cst->cnt; i = 0; }
  else             { z = -1;       i = cst->cnt-1; }
  #endif                        /* (except all processing fits) */
  for (r = 0; i != z; i += rd->dir) {
    h = cst->heads +i;          /* traverse the frequent items */
    r = isr_add(rd->report, h->item, h->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    if (!h->list->succ) {       /* if projection would be a chain */
      for (node = h->list->parent; node->id >= 0; ) {
        isr_addpex(rd->report, cst->heads[node->id].item);
        node = node->parent;    /* traverse the list of ancestors */
      } }                       /* and add them as perfect exts. */
    else if (proj) {            /* if another item can be added */
      r = (rd->mode & FPG_REORDER)
        ? proj_reord(proj, cst, i, rd)
        : proj_cmplx(proj, cst, i, rd);
      if (r > 0) r = rec_cmplx(proj, rd);
      if (r < 0) break;         /* project frequent pattern tree and */
    }                           /* find freq. item sets recursively */
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  if (proj) {                   /* delete the created projection */
    free(proj); ms_pop(cst->mem); }
  return r;                     /* return the error status */
}  /* rec_cmplx() */

/*--------------------------------------------------------------------*/

int fpg_cmplx (TABAG *tabag, int target, SUPP smin, int mode,
               ISREPORT *report)
{                               /* --- search for frequent item sets */
  int        r = 0;             /* result of recursion/functions */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        j, n;              /* loop variable, number of trans. */
  SUPP       pex;               /* minimum support for perfect exts. */
  ITEM       *s, *d;            /* to build the item maps */
  const ITEM *p;                /* to traverse transaction items */
  const SUPP *f;                /* item frequencies in trans. bag */
  TRACT      *t;                /* to traverse the transactions */
  CSTREE     *cst;              /* created frequent pattern tree */
  CSHEAD     *h;                /* to traverse the item heads */
  RECDATA    rd;                /* structure for recursive search */

  assert(tabag && report);      /* check the function arguments */
  rd.mode = mode;               /* store search mode and item dir. */
  rd.dir  = (target & (ISR_CLOSED|ISR_MAXIMAL)) ? -1 : +1;
  rd.smin = (smin > 0) ? smin : 1;  /* check and adapt the support */
  pex     = tbg_wgt(tabag);     /* check against the minimum support */
  if (rd.smin > pex) return 0;  /* and get minimum for perfect exts. */
  if (!(mode & FPG_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  f = tbg_ifrqs(tabag, 0);      /* get the item frequencies */
  if (!f) return -1;            /* in the transaction bag */
  s = rd.set = (ITEM*)malloc((size_t)(k+k) *sizeof(ITEM)
                            +(size_t) k    *sizeof(SUPP));
  if (!s) return -1;            /* create item and support arrays */
  rd.map = d = s+k;             /* note item map and set buffer */
  rd.cis = (SUPP*)(d+k);        /* and the item support array */
  for (i = m = 0; i < k; i++) { /* build the item identifier map */
    if (f[i] <  rd.smin) { d[i] = -1;                        continue; }
    if (f[i] >= pex)     { d[i] = -1; isr_addpex(report, i); continue; }
    d[i] = m; s[m++] = i;       /* eliminate infrequent items and */
  }                             /* collect perfect extension items */
  if (m <= 0) {                 /* check whether there are items left */
    r = isr_report(report); free(rd.set); return r; }
  cst = (CSTREE*)malloc(sizeof(CSTREE) +(size_t)(m-1) *sizeof(CSHEAD));
  if (!cst) { free(rd.set); return -1; }
  cst->cnt = k = m;             /* allocate the base tree structure */
  cst->mem = ms_create(sizeof(CSNODE), 65535);
  if (!cst->mem) { free(cst); free(rd.set); return -1; }
  cst->root.id      = TA_END;   /* create memory system for the nodes */
  cst->root.supp    = 0;        /* and initialize the root node */
  cst->root.sibling = cst->root.children = NULL;
  cst->root.succ    = cst->root.parent   = NULL;
  for (i = 0; i < k; i++) {     /* initialize the header table */
    h = cst->heads+i; h->supp = f[h->item = s[i]]; h->list = NULL; }
  rd.fim16 = NULL;              /* default: no 16-items machine */
  if (mode & FPG_FIM16) {       /* if to use a 16-items machine */
    rd.fim16 = m16_create(rd.dir, rd.smin, report);
    if (!rd.fim16) { ms_delete(cst->mem);
      free(cst); free(rd.set); return -1; }
  }                             /* create a 16-items machine */
  for (j = n; --j >= 0; ) {     /* traverse the transactions and */
    t = tbg_tract(tabag, j);    /* collect the non-eliminated items */
    for (k = 0, p = ta_items(t); *p > TA_END; p++)
      if ((m = d[*p]) >= 0) s[k++] = m;
    r = add_cmplx(cst, s, k, ta_wgt(t));
    if (r < 0) break;           /* add the reduced transaction */
  }                             /* to the frequent pattern tree */
  if (r >= 0) {                 /* if a frequent pattern tree */
    rd.report = report;         /* has successfully been built, */
    r = rec_cmplx(cst, &rd);    /* find freq. item sets recursively */
    if (r >= 0) r = isr_report(report);
  }                             /* report the empty item set */
  if (rd.fim16)                 /* if a 16-items machine was used, */
    m16_delete(rd.fim16);       /* delete the 16-items machine */
  ms_delete(cst->mem);          /* delete the memory mgmt. system */
  free(cst); free(rd.set);      /* and the frequent pattern tree */
  return r;                     /* return the error status */
}  /* fpg_cmplx() */

/*----------------------------------------------------------------------
  Frequent Pattern Growth (on single tree with simple nodes)
----------------------------------------------------------------------*/

static int rec_single (FPTREE *fpt, ITEM n, RECDATA *rd)
{                               /* --- search for frequent item sets */
  int    r;                     /* error status */
  ITEM   i, k, m;               /* loop variables */
  SUPP   pex;                   /* minimum support for perfect exts. */
  FPHEAD *h;                    /* header for current item */
  FPNODE *node, *anc;           /* to traverse the tree nodes */

  assert(fpt && rd);            /* check the function arguments */
  #ifdef FPG_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  i = (fpt->fim16) ? 1 : 0;     /* skip packed items if they exist */
  for (r = 0; i < n; i++) {     /* traverse the (other) items, */
    h = fpt->heads +i;          /* but skip infrequent items */
    if (h->supp < rd->smin) continue;
    r = isr_add(rd->report, h->item, h->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) continue;       /* check if item needs processing */
    node = h->list;             /* get (first node of) item list */
    if (!node->succ) {          /* if projection would be a chain */
      for (anc = node->parent; anc->id > TA_END; anc = anc->parent) {
        k = anc->id;            /* traverse the list of ancestors */
        if (k >= 0) isr_addpex  (rd->report, fpt->heads[k].item);
        else        isr_addpexpk(rd->report, k);
      } }                       /* add items as perfect extensions */
    else if ((i > 0)            /* if another item can be added */
    &&       isr_xable(rd->report, 1)) {
      for (k = 0; k < i; k++) { /* clear item heads and support */
        h = fpt->heads +k; h->supp = 0; h->list = NULL; }
      for ( ; node; node = node->succ) {
        for (anc = node->parent; anc->id > TA_END; anc = anc->parent) {
          if (anc->id < 0)      /* traverse the list of ancestors */
            m16_add(fpt->fim16, (BITTA)(anc->id & ~TA_END), node->supp);
          else {                /* add packed items to 16-items mach. */
            h = fpt->heads +anc->id;    /* traverse the item list */
            if (h->list == anc) break;  /* and the paths to the root */
            h->supp  += anc->supp = node->supp;
            anc->succ = h->list;/* store and sum the node support */
            h->list   = anc;    /* and insert the current ancestor */
          }                     /* into the corresp. item list */
        }
        for ( ; anc->id > TA_END; anc = anc->parent) {
          if (anc->id < 0)      /* traverse the rest of the list */
            m16_add(fpt->fim16, (BITTA)(anc->id & ~TA_END), node->supp);
          else {                /* add packed items to 16-items mach. */
            fpt->heads[anc->id].supp += node->supp;
            anc->supp += node->supp;
          }                     /* update the support values */
        }                       /* on the rest of the path */
      }
      pex = (rd->mode & FPG_PERFECT) ? fpt->heads[i].supp : SUPP_MAX;
      k = (fpt->fim16) ? 1 : 0; /* skip packed items if they exist */
      for (m = 0; k < i; k++) { /* traverse the (other) items again, */
        h = fpt->heads +k;      /* but skip infrequent items */
        if (h->supp < rd->smin)   continue;
        if (h->supp < pex) { m++; continue; }
        h->supp = 0;            /* count the frequent items */
        isr_addpex(rd->report, h->item);
      }                         /* collect the perfect extensions */
      if (fpt->fim16) {         /* if there is a 16-items machine */
        r = m16_mine(fpt->fim16);
        if (r < 0) { m = 0; break; }
      }                         /* mine frequent item sets */
      if (m > 0) r = rec_single(fpt, i, rd);
      if (r < 0) break;         /* if the projection is not empty, */
    }                           /* process it recursively */
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
  }                             /* from the item set reporter */
  return r;                     /* return the error status */
}  /* rec_single() */

/*--------------------------------------------------------------------*/

int fpg_single (TABAG *tabag, int target, SUPP smin, int mode,
                ISREPORT *report)
{                               /* --- search for frequent item sets */
  int        r = 0;             /* result of recursion/functions */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        j, n;              /* loop variable, number of trans. */
  SUPP       pex;               /* minimum support for perfect exts. */
  ITEM       *s, *d;            /* to build the item maps */
  const ITEM *p;                /* to traverse transaction items */
  const SUPP *f;                /* item frequencies in trans. bag */
  TRACT      *t;                /* to traverse the transactions */
  FPTREE     *fpt;              /* created frequent pattern tree */
  FPHEAD     *h;                /* to traverse the item heads */
  RECDATA    rd;                /* structure for recursive search */

  assert(tabag && report);      /* check the function arguments */
  rd.mode = mode;               /* store search mode and item dir. */
  rd.dir  = +1;                 /* (only upward item loops possible) */
  rd.smin = (smin > 0) ? smin : 1;    /* check and adapt the support */
  pex     = tbg_wgt(tabag);     /* check against the minimum support */
  if (rd.smin > pex) return 0;  /* and get minimum for perfect exts. */
  if (!(mode & FPG_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  f = tbg_ifrqs(tabag, 0);      /* get the item frequencies */
  if (!f) return -1;            /* in the transaction bag */
  s = rd.set = (ITEM*)malloc((size_t)(k+k) *sizeof(ITEM));
  if (!s) return -1;            /* create item and support arrays */
  rd.map = d = s+k;             /* note item map and set buffer */
  if (!(mode & FPG_FIM16)) m = 0;   /* if to use a 16-items machine, */
  else { d[0] = s[0] = 0;  m = 1; } /* always keep the packed items */
  for (i = m; i < k; i++) {     /* build the item identifier map */
    if (f[i] <  rd.smin) { d[i] = -1;                        continue; }
    if (f[i] >= pex)     { d[i] = -1; isr_addpex(report, i); continue; }
    d[i] = m; s[m++] = i;       /* eliminate infrequent items and */
  }                             /* collect perfect extension items */
  if (m <= 0) {                 /* check whether there are items left */
    r = isr_report(report); free(rd.set); return r; }
  fpt = (FPTREE*)malloc(sizeof(FPTREE) +(size_t)(m-1) *sizeof(FPHEAD));
  if (!fpt) { free(rd.set); return -1; }
  fpt->cnt = k = m;             /* allocate the base tree structure */
  fpt->dir = rd.dir;            /* and initialize its fields */
  fpt->mem = ms_create(sizeof(FPNODE), 65535);
  if (!fpt->mem) { free(fpt); free(rd.set); return -1; }
  fpt->root.id   = TA_END;      /* create memory system for the nodes */
  fpt->root.supp = 0;           /* and initialize the root node */
  fpt->root.succ = fpt->root.parent = NULL;
  fpt->fim16 = NULL;            /* default: no 16-items machine */
  if (mode & FPG_FIM16) {       /* if to use a 16-items machine */
    fpt->fim16 = m16_create(rd.dir, rd.smin, report);
    if (!fpt->fim16) { ms_delete(fpt->mem);
      free(fpt); free(rd.set); return -1; }
  }                             /* create a 16-items machine */
  for (i = 0; i < k; i++) {     /* initialize the item heads */
    h = fpt->heads+i; h->supp = f[h->item = s[i]]; h->list = NULL; }
  for (j = n; --j >= 0; ) {     /* traverse the transactions and */
    t = tbg_tract(tabag, j);    /* collect the non-eliminated items */
    for (k = 0, p = ta_items(t); *p > TA_END; p++) {
      if      ((m = *p)   <  0) s[k++] = m;
      else if ((m = d[m]) >= 0) s[k++] = m;
    }                           /* add packed items to 16-items mach. */
    r = add_smp16(fpt, s, k, ta_wgt(t));
    if (r < 0) break;           /* add the reduced transaction */
  }                             /* to the frequent pattern tree */
  if ((r >= 0) && fpt->fim16)   /* if there is a 16-items machine, */
    r = m16_mine(fpt->fim16);   /* mine frequent item sets with it */
  if (r >= 0) {                 /* if a frequent pattern tree */
    rd.report = report;         /* has successfully been built */
    r = rec_single(fpt, fpt->cnt, &rd);
    if (r >= 0) r = isr_report(report);
  }                             /* find freq. item sets recursively */
  if (fpt->fim16)               /* if a 16-items machine was used, */
    m16_delete(fpt->fim16);     /* delete the 16-items machine */
  ms_delete(fpt->mem);          /* delete the memory mgmt. system */
  free(fpt); free(rd.set);      /* and the frequent pattern tree */
  return r;                     /* return the error status */
}  /* fpg_single() */

/*----------------------------------------------------------------------
  Frequent Pattern Growth (top-down processing)
----------------------------------------------------------------------*/

static int add_topdn (TDTREE *tdt, const ITEM *ids, ITEM n, SUPP supp)
{                               /* --- add an item set to the tree */
  ITEM   i;                     /* buffer for an item */
  TDNODE **p;                   /* insertion position */
  TDNODE *node;                 /* to insert new nodes */

  assert(tdt                    /* check the function arguments */
  &&    (ids || (n <= 0)) && (supp >= 0));
  p = &tdt->root;               /* start the search at the root node */
  while (1) {                   /* traverse the items of the set */
    if (--n < 0) return 0;      /* if all items are processed, abort */
    i = ids[n];                 /* get the next item in the set */
    while (*p && ((*p)->id > i)) p = &(*p)->sibling;
    node = *p;                  /* find the item/insertion position */
    if (!node || (node->id != i)) break;
    node->supp += supp;         /* if the item does not exist, abort */
    p = &node->children;        /* else update the item set support */
  }                             /* and get the list of children */
  node = (TDNODE*)ms_alloc(tdt->mem);
  if (!node) return -1;         /* create a new prefix tree node */
  node->id      = i;            /* store the current item and */
  node->supp    = supp;         /* the support of the item set */
  node->sibling = *p;           /* insert the created node */
  *p = node;                    /* into the sibling list */
  while (--n >= 0) {            /* traverse the rest of the items */
    node = node->children = (TDNODE*)ms_alloc(tdt->mem);
    if (!node) return -1;       /* create a new prefix tree node */
    node->id      = ids[n];     /* create a new prefix tree node */
    node->supp    = supp;       /* store item and its support */
    node->sibling = NULL;       /* there are no siblings yet */
  }
  node->children = NULL;        /* last created node is a leaf */
  return 1;                     /* return that nodes were added */
}  /* add_topdn() */

/*--------------------------------------------------------------------*/

static void getsupp (TDNODE *node, SUPP *supp)
{                               /* --- determine conditional support */
  for ( ; node; node = node->sibling) {
    supp[node->id] += node->supp;
    getsupp(node->children, supp);
  }                             /* recursively sum support per item */
}  /* getsupp() */

/*--------------------------------------------------------------------*/

static TDNODE* merge (TDNODE *s1, TDNODE *s2)
{                               /* --- merge two node lists */
  TDNODE *out, **end = &out;    /* output node list and end pointer */

  if (!s1) return s2;           /* if there is only one node list, */
  if (!s2) return s1;           /* simply return the other list */
  end = &out;                   /* start the output list */
  while (1) {                   /* node list merge loop */
    if      (s1->id > s2->id) { /* copy node with singular item */
      *end = s1; end = &s1->sibling; s1 = *end; if (!s1) break; }
    else if (s2->id > s1->id) { /* copy node with singular item */
      *end = s2; end = &s2->sibling; s2 = *end; if (!s2) break; }
    else {                      /* if item occurs in both trees */
      s1->children = merge(s1->children, s2->children);
      s1->supp += s2->supp;     /* merge the children recursively */
      *end = s1; end = &s1->sibling; s1 = *end; s2 = s2->sibling;
      if (!s1 || !s2) break;    /* move node from the first source */
    }                           /* to the output and delete the node */
  }                             /* from the second source */
  *end = (s1) ? s1 : s2;        /* append the remaining nodes */
  return out;                   /* return the merged top-down tree */
}  /* merge() */

/*--------------------------------------------------------------------*/

static TDNODE* copy (TDNODE *src, ITEM *map, MEMSYS *mem)
{                               /* --- copy a top-down tree */
  ITEM   i;                     /* new item identifier */
  TDNODE *node, *dst;           /* created copy of the node list */
  TDNODE **end = &dst;          /* end of the created copy */
  TDNODE *c, *b = NULL;         /* buffer for copied children */

  assert(src && map && mem);    /* check the function arguments */
  do {                          /* sibling list copying loop */
    c = src->children;          /* if there are children, copy them */
    if (c && ((c = copy(c, map, mem)) == COPYERR)) return COPYERR;
    i = map[src->id];           /* get the new item identifier */
    if (i >= 0) {               /* if to copy the current node */
      *end = node = (TDNODE*)ms_alloc(mem);
      if (!node) return COPYERR;/* create a copy of the current node */
      node->id   = i;           /* store the item and the support and */
      node->supp = src->supp;   /* update the conditional support */
      node->children = c;       /* set the (copied) children */
      end = &node->sibling; }   /* get the new end of the output */
    else if (c)                 /* merge copied children to a buffer */
      b = (!b) ? c : merge(b, c);
    src = src->sibling;         /* get the next sibling */
  } while (src);                /* while there is another node */
  *end = NULL;                  /* terminate the copied list */
  return (!b) ? dst : (!dst) ? b : merge(dst, b);
}  /* copy() */                 /* merge with buffered copies */

/*--------------------------------------------------------------------*/

static int rec_topdn (TDTREE *tdt, RECDATA *rd)
{                               /* --- find item sets recursively */
  int    r = 0;                 /* error status */
  ITEM   i, k;                  /* loop variables */
  SUPP   pex;                   /* minimum for perfect extensions */
  TDTREE *proj = NULL;          /* created projection */
  TDNODE *node;                 /* to traverse the nodes */
  SUPP   *s;                    /* to compute the item support */
  ITEM   *map;                  /* to build the item map */

  assert(tdt && rd);            /* check the function arguments */
  #ifdef FPG_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (rd->mode & FPG_TAIL) {    /* if to use head union tail pruning */
    r = isr_tail(rd->report, tdt->items, tdt->cnt);
    if (r) return r;            /* if tail needs no processing, */
  }                             /* abort the recursion */
  if ((tdt->cnt > 1)            /* if there is more than one item */
  &&  isr_xable(rd->report,2)){ /* and another item can be added */
    proj = (TDTREE*)malloc(sizeof(TDTREE)
                         +(size_t)(tdt->cnt-2) *sizeof(ITEM));
    if (!proj) return -1;       /* create a frequent pattern tree */
    proj->mem = tdt->mem;       /* of the maximally possible size */
    if (ms_push(tdt->mem) < 0) { free(proj); return -1; }
  }                             /* note the current memory state */
  for (node = tdt->root; node; node = tdt->root) {
    r = isr_add(rd->report, tdt->items[node->id], node->supp);
    if (r <  0) break;          /* add current item to the reporter */
    if (r <= 0) {               /* check if item needs processing */
      tdt->root = merge(node->sibling, node->children); continue; }
    if (proj && node->children){/* if current node has children */
      memset(s = rd->cis, 0, (size_t)node->id *sizeof(SUPP));
      getsupp(node->children,s);/* determine the conditional support */
      pex = (rd->mode & FPG_PERFECT) ? node->supp : SUPP_MAX;
      map = rd->map;            /* get perfect extension support */
      for (i = k = 0; i < node->id; i++) {
        if (s[i] <  rd->smin) { /* traverse items and their support */
          map[i] = -1; continue; } /* eliminate infrequent items and */
        if (s[i] >= pex) {         /* collect perfect extension items */
          map[i] = -1; isr_addpex(rd->report, tdt->items[i]); continue;}
        map[i] = k; proj->items[k++] = tdt->items[i];
      }                         /* build item identifier maps */
      if (k > 0) {              /* if the projection is not empty, */
        proj->cnt  = k;         /* note the number of items */
        proj->root = copy(node->children, map, proj->mem);
        if (proj->root == COPYERR) { r = -1; break; }
        r = rec_topdn(proj, rd);
        if (r < 0) break;       /* copy the subtree for the item */
      }                         /* and process it recursively */
    }
    r = isr_report(rd->report); /* report the current item set */
    if (r < 0) break;           /* and check for an error */
    isr_remove(rd->report, 1);  /* remove the current item */
    tdt->root = merge(node->sibling, node->children);
  }                             /* prune the processed item */
  if (proj) {                   /* delete the created projection */
    free(proj); ms_pop(tdt->mem); }
  return r;                     /* return the error status */
}  /* rec_topdn() */

/*--------------------------------------------------------------------*/

int fpg_topdn (TABAG *tabag, int target, SUPP smin, int mode,
               ISREPORT *report)
{                               /* --- search for frequent item sets */
  int        r = 0;             /* result of recursion/functions */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        j, n;              /* loop variable, number of trans. */
  SUPP       pex;               /* minimum support for perfect exts. */
  TRACT      *t;                /* to traverse the transactions */
  ITEM       *s, *d;            /* to traverse flags / items */
  const ITEM *p;                /* to traverse transaction items */
  const SUPP *f;                /* item frequencies in trans. bag */
  TDTREE     *tdt;              /* top-down prefix tree */
  RECDATA    rd;                /* structure for recursive search */

  assert(tabag && report);      /* check the function arguments */
  rd.mode = mode;               /* store search mode and item dir. */
  rd.dir  = +1;                 /* (only upward item loops possible) */
  rd.smin = (smin > 0) ? smin : 1;    /* check and adapt the support */
  pex     = tbg_wgt(tabag);     /* check against the minimum support */
  if (rd.smin > pex) return 0;  /* and get minimum for perfect exts. */
  if (!(mode & FPG_PERFECT)) pex = SUPP_MAX;
  n = tbg_cnt(tabag);           /* get the number of transactions */
  k = tbg_itemcnt(tabag);       /* and check the number of items */
  if (k <= 0) return isr_report(report);
  f = tbg_ifrqs(tabag, 0);      /* get the item frequencies */
  if (!f) return -1;            /* in the transaction bag */
  s = rd.set = (ITEM*)malloc((size_t)(k+k) *sizeof(ITEM)
                            +(size_t) k    *sizeof(SUPP));
  if (!s) return -1;            /* create item and support arrays */
  rd.map = d = s+k;             /* note item set buffer and item map */
  rd.cis = (SUPP*)(d+k);        /* and the item support array */
  for (i = m = 0; i < k; i++) { /* build the item identifier map */
    if (f[i] <  rd.smin) { d[i] = -1;                        continue; }
    if (f[i] >= pex)     { d[i] = -1; isr_addpex(report, i); continue; }
    d[i] = m; s[m++] = i;       /* eliminate infrequent items and */
  }                             /* collect perfect extension items */
  if (m <= 0) {                 /* check whether there are items left */
    r = isr_report(report); free(rd.set); return r; }
  tdt = (TDTREE*)malloc(sizeof(TDTREE) +(size_t)(m-1) *sizeof(ITEM));
  if (!tdt) { free(rd.set); return -1; }
  tdt->cnt  = k = m;            /* create a top-down prefix tree and */
  tdt->root = NULL;             /* the memory system for the nodes */
  tdt->mem  = ms_create(sizeof(TDNODE), 65535);
  if (!tdt->mem) { free(tdt); free(rd.set); return -1; }
  memcpy(tdt->items, d, (size_t)k *sizeof(ITEM));
  for (j = n; --j >= 0; ) {     /* traverse the transactions and */
    t = tbg_tract(tabag, j);    /* collect the non-eliminated items */
    for (k = 0, p = ta_items(t); *p > TA_END; p++)
      if ((m = d[*p]) >= 0) s[k++] = m;
    r = add_topdn(tdt, s, k, ta_wgt(t));
    if (r < 0) break;           /* add the reduced transaction */
  }                             /* to the frequent pattern tree */
  if (r >= 0) {                 /* if a frequent pattern tree */
    rd.report = report;         /* has successfully been built, */
    r = rec_topdn(tdt, &rd);    /* find freq. item sets recursively */
    if (r >= 0) r = isr_report(report);
  }                             /* report the empty item set */
  ms_delete(tdt->mem);          /* delete the memory mgmt. system */
  free(tdt); free(rd.set);      /* delete the frequent pattern tree */
  return r;                     /* return the error status */
}  /* fpg_topdn() */

/*----------------------------------------------------------------------
  Frequent Pattern Growth (on single tree, for rules)
----------------------------------------------------------------------*/

int rec_tree (FPTREE *fpt, ITEM n, RECDATA *rd)
{                               /* --- search for frequent item sets */
  int    r = 0;                 /* error status */
  ITEM   i, k, m;               /* loop variables */
  FPHEAD *h;                    /* header for current item */
  FPNODE *node, *anc;           /* to traverse the tree nodes */

  assert(fpt && rd);            /* check the function arguments */
  #ifdef FPG_ABORT              /* if to check for interrupt */
  if (sig_aborted()) return -1; /* if execution was aborted, */
  #endif                        /* abort the recursion */
  if (!ist_xable(rd->istree, 1))
    return 0;                   /* if no item can be added, abort */
  if (ist_addchn(rd->istree) != 0)
    return -1;                  /* add children to current node */
  for (i = 1; i < n; i++) {     /* traverse the items, */
    h = fpt->heads +i;          /* but skip the infrequent items */
    if (h->supp < rd->smin) continue;
    if (ist_down(rd->istree, h->item) < 0)
      continue;                 /* go down in the item set tree */
    for (k = 0; k < i; k++) {   /* clear item heads and support */
      h = fpt->heads +k; h->supp = 0; h->list = NULL; }
    for (node = fpt->heads[i].list; node; node = node->succ) {
      for (anc = node->parent; anc->id > TA_END; anc = anc->parent) {
        h = fpt->heads +anc->id;    /* traverse the item list */
        if (h->list == anc) break;  /* and the paths to the root */
        h->supp  += anc->supp = node->supp;
        anc->succ = h->list;    /* store and sum the node support */
        h->list   = anc;        /* and insert the current ancestor */
      }                         /* into the corresp. item list */
      for ( ; anc->id > TA_END; anc = anc->parent) {
        fpt->heads[anc->id].supp += node->supp;
        anc->supp += node->supp;
      }                         /* update the support values */
    }                           /* on the rest of the path */
    for (k = m = 0; k < i; k++) {
      h = fpt->heads +k;        /* traverse the items */
      if (h->supp < rd->smin) continue;
      ist_setsupp(rd->istree, h->item, h->supp);
      m += 1;                   /* set the item set support and */
    }                           /* count the frequent items */
    if (m > 0) r = rec_tree(fpt, i, rd);
    if (r < 0) break;           /* if the projection is not empty, */
    ist_up(rd->istree);         /* process it recursively, */
  }                             /* then go back up in the tree */
  return r;                     /* return the error status */
}  /* rec_tree() */

/*--------------------------------------------------------------------*/

int fpg_tree (TABAG *tabag, SUPP smin, int mode, ISTREE *istree)
{                               /* --- search for frequent item sets */
  int        r = 0;             /* result of recursion/functions */
  ITEM       i, k, m;           /* loop variable, number of items */
  TID        j, n;              /* loop variable, number of trans. */
  ITEM       *s, *d;            /* to build the item maps */
  const ITEM *p;                /* to traverse transaction items */
  const SUPP *f;                /* item frequencies in trans. bag */
  TRACT      *t;                /* to traverse the transactions */
  FPTREE     *fpt;              /* created frequent pattern tree */
  FPHEAD     *h;                /* to traverse the item heads */
  RECDATA    rd;                /* structure for recursive search */

  assert(tabag && istree);      /* check the function arguments */
  rd.mode = mode;               /* store search mode and item dir. */
  rd.dir  = +1;                 /* (only upward item loops possible) */
  rd.smin = (smin > 0) ? smin : 1;    /* check and adapt the support */
  k = tbg_itemcnt(tabag);       /* get the number of (frequent) items */
  if (k <= 0) return 0;         /* if there are none, abort */
  f = tbg_ifrqs(tabag, 0);      /* get the item frequencies */
  if (!f) return -1;            /* in the transaction bag */
  s = rd.set = (ITEM*)malloc((size_t)(k+k) *sizeof(ITEM));
  if (!s) return -1;            /* create item and support arrays */
  rd.map = d = s+k;             /* note item map and set buffer */
  for (i = m = 0; i < k; i++) { /* build the item identifier map */
    if (f[i] < rd.smin) { d[i] = -1; }
    else                { d[i] =  m; s[m++] = i; }
  }                             /* collect the frequent items */
  if (m <= 0) { free(rd.set); return r; }
  fpt = (FPTREE*)malloc(sizeof(FPTREE) +(size_t)(m-1) *sizeof(FPHEAD));
  if (!fpt) { free(rd.set); return -1; }
  fpt->cnt = k = m;             /* allocate the base tree structure */
  fpt->dir = rd.dir;            /* and initialize its fields */
  fpt->mem = ms_create(sizeof(FPNODE), 65535);
  if (!fpt->mem) { free(fpt); free(rd.set); return -1; }
  fpt->root.id   = TA_END;      /* create memory system for the nodes */
  fpt->root.supp = 0;           /* and initialize the root node */
  fpt->root.succ = fpt->root.parent = NULL;
  for (i = 0; i < k; i++) {     /* initialize the item heads */
    h = fpt->heads+i; h->supp = f[h->item = s[i]]; h->list = NULL; }
  n = tbg_cnt(tabag);           /* get the number of transactions */
  for (j = n; --j >= 0; ) {     /* traverse the transactions and */
    t = tbg_tract(tabag, j);    /* collect the non-eliminated items */
    for (k = 0, p = ta_items(t); *p > TA_END; p++) {
      if      ((m = *p)   <  0) s[k++] = m;
      else if ((m = d[m]) >= 0) s[k++] = m;
    }                           /* add packed items to 16-items mach. */
    r = add_simple(fpt, s, k, ta_wgt(t));
    if (r < 0) break;           /* add the reduced transaction */
  }                             /* to the frequent pattern tree */
  if (r >= 0) {                 /* if a frequent pattern tree */
    rd.istree = istree;         /* has successfully been built */
    r = rec_tree(fpt, fpt->cnt, &rd);
  }                             /* find freq. item sets recursively */
  ms_delete(fpt->mem);          /* delete the memory mgmt. system */
  free(fpt); free(rd.set);      /* and the frequent pattern tree */
  return r;                     /* return the error status */
}  /* fpg_tree() */

/*----------------------------------------------------------------------
  Frequent Pattern Growth (generic)
----------------------------------------------------------------------*/

static FPGFN* fpgvars[] = {     /* --- table of fp-growth variants */
  fpg_simple,                   /* simple  nodes (parent/successor) */
  fpg_cmplx,                    /* complex nodes (children/sibling) */
  fpg_single,                   /* top-down processing w/ single tree */
  fpg_topdn,                    /* top-down processing of the tree */
};

/*--------------------------------------------------------------------*/

int fpg_data (TABAG *tabag, int target, SUPP smin, ITEM zmin,
              int eval, int algo, int mode, int sort)
{                               /* --- prepare data for FP-growth */
  ITEM    m;                    /* number of items */
  ITEM    pack;                 /* number of items to pack */
  #ifndef QUIET                 /* if to print messages */
  TID     n;                    /* number of transactions */
  SUPP    w;                    /* total transaction weight */
  clock_t t;                    /* timer for measurements */
  #endif                        /* (only needed for messages) */

  assert(tabag);                /* check the function arguments */

  /* --- make parameters consistent --- */
  if      (target & ISR_RULES)   target = ISR_RULES;
  else if (target & ISR_GENERAS) target = ISR_GENERAS;
  else if (target & ISR_MAXIMAL) target = ISR_MAXIMAL;
  else if (target & ISR_CLOSED)  target = ISR_CLOSED;
  else                           target = ISR_ALL;
  eval &= ~FPG_INVBXS;          /* get target and remove flags */
  if (target == ISR_GENERAS) {  /* if to filter for generators, */
    if (algo == FPG_TOPDOWN) algo = FPG_SINGLE; }
  else if (target & (ISR_CLOSED|ISR_MAXIMAL)) {
    mode &= ~FPG_REORDER;       /* reordering only for all item sets */
    if (algo == FPG_SINGLE) algo = FPG_SIMPLE;
  }                             /* not all variants work for filter */
  if (algo != FPG_COMPLEX)      /* reordering/recoding of items */
    mode &= ~FPG_REORDER;       /* only for complex trees */
  if ((algo != FPG_SIMPLE) && (algo != FPG_COMPLEX)
  &&  (algo != FPG_SINGLE))     /* not all algorithm variants */
    mode &= ~FPG_FIM16;         /* support a 16-items machine */
  if ((target & ISR_RULES) || ((eval > RE_NONE) && (eval < RE_FNCNT)))
    mode &= ~FPG_FIM16;         /* eval. does not support packing */
  pack = mode & FPG_FIM16;      /* get number of items to pack */
  if (pack > 16) pack = 16;     /* pack at most 16 items */
  if (algo == FPG_COMPLEX)      /* for complex fp-trees, if needed, */
    pack = 0;                   /* items are packed in the recursion */
  if (mode & FPG_REORDER)       /* simplified sorting if reordering */
    sort = (sort < 0) ? -1 : (sort > 0) ? +1 : 0;

  /* --- sort and recode items --- */
  CLOCK(t);                     /* start timer, print log message */
  XMSG(stderr, "filtering, sorting and recoding items ... ");
  m = tbg_recode(tabag, smin, -1, -1, -sort);
  if (m <  0) return E_NOMEM;   /* recode items and transactions */
  if (m <= 0) return E_NOITEMS; /* and check the number of items */
  XMSG(stderr, "[%"ITEM_FMT" item(s)]", m);
  XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- sort and reduce transactions --- */
  CLOCK(t);                     /* start timer, print log message */
  XMSG(stderr, "sorting and reducing transactions ... ");
  if (!(target & ISR_RULES)     /* filter transactions if possible */
  &&  ((eval <= RE_NONE) || (eval >= RE_FNCNT)))
    tbg_filter(tabag, zmin, NULL, 0);
  tbg_itsort(tabag, +1, 0);     /* sort items in transactions and */
  tbg_sort  (tabag, +1, 0);     /* sort the trans. lexicographically */
  tbg_reduce(tabag, 0);         /* reduce transactions to unique ones */
  if (pack > 0)                 /* if to use a 16-items machine, */
    tbg_pack(tabag, pack);      /* pack the most frequent items */
  #ifndef QUIET                 /* if to print messages */
  n = tbg_cnt(tabag);           /* get the number of transactions */
  w = tbg_wgt(tabag);           /* and the transaction weight */
  XMSG(stderr, "[%"TID_FMT, n); /* print number of transactions */
  if (w != (SUPP)n) { XMSG(stderr, "/%"SUPP_FMT, w); }
  XMSG(stderr, " transaction(s)] done [%.2fs].\n", SEC_SINCE(t));
  #endif
  return 0;                     /* return 'ok' */
}  /* fpg_data() */

/*--------------------------------------------------------------------*/

int fpg_repo (ISREPORT *report, int target,
              int eval, double thresh, int algo, int mode)
{                               /* --- prepare reporter for FP-growth */
  int mrep;                     /* mode for item set reporter */

  assert(report);               /* check the function arguments */

  /* --- make parameters consistent --- */
  if      (target & ISR_RULES)   target = ISR_RULES;
  else if (target & ISR_GENERAS) target = ISR_GENERAS;
  else if (target & ISR_MAXIMAL) target = ISR_MAXIMAL;
  else if (target & ISR_CLOSED)  target = ISR_CLOSED;
  else                           target = ISR_ALL;
  mrep = 0;                     /* get target and init. repo. mode */
  if ((algo != FPG_COMPLEX) || (target & (ISR_CLOSED|ISR_MAXIMAL)))
    mode &= ~FPG_REORDER;       /* cannot reorder for closed/maximal */
  if ((target & ISR_GENERAS) && (mode & FPG_REORDER))
    mrep |= ISR_SORT;           /* reordering requires set sorting */
  eval &= ~FPG_INVBXS;          /* remove flags from measure code */
  if ((target & ISR_RULES) || ((eval > RE_NONE) && (eval < RE_FNCNT)))
    mrep |= ISR_NOFILTER;       /* no filtering if done in FP-growth */

  /* --- configure item set reporter --- */
  if (eval == FPG_LDRATIO)      /* set additional evaluation measure */
    isr_seteval(report, isr_logrto, NULL, +1, thresh);
  return (isr_settarg(report, target, mrep, -1)) ? E_NOMEM : 0;
}  /* fpg_repo() */

/*--------------------------------------------------------------------*/

static int cleanup (FPGROWTH *data)
{                               /* --- clean up on error */
  if (data->mode & FPG_NOCLEAN) return E_NOMEM;
  if (data->istree) ist_delete(data->istree);
  return E_NOMEM;               /* free all allocated memory */
}  /* cleanup() */              /* return an error indicator */

/*--------------------------------------------------------------------*/

int fpgrowth (TABAG *tabag, int target, SUPP smin, SUPP body,
              double conf, int eval, int agg, double thresh,
              ITEM prune, int algo, int mode,
              int order, ISREPORT *report)
{                               /* --- fpgrowth algorithm */
  int      r;                   /* result of function call */
  ITEM     m;                   /* number of items */
  int      e;                   /* evaluation without flags */
  ITEM     zmin, zmax;          /* number of items in set/rule */
  ITEMBASE *base;               /* underlying item base */
  FPGROWTH a = { 0, NULL };     /* fpgrowth execution data */
  #ifndef QUIET                 /* if to print messages */
  clock_t  t;                   /* timer for measurements */
  #endif                        /* (only needed for messages) */

  assert(tabag && report);      /* check the function arguments */
  a.mode = mode;                /* note the processing mode */

  /* --- make algorithm and modes consistent --- */
  if (algo != FPG_COMPLEX)      /* reordering/recoding of items */
    mode &= ~FPG_REORDER;       /* only for complex trees */
  if (target == ISR_GENERAS) {  /* if to filter for generators, */
    mode |=  FPG_PERFECT;       /* need perfect extension pruning */
    if (algo == FPG_TOPDOWN) algo = FPG_SINGLE; }
  else if (target & (ISR_CLOSED|ISR_MAXIMAL)) {
    mode &= ~FPG_REORDER;       /* reordering only for all item sets */
    if (algo == FPG_SINGLE)  algo = FPG_SIMPLE;
  }                             /* not all variants work for filter */
  if ((algo != FPG_SIMPLE) && (algo != FPG_COMPLEX)
  &&  (algo != FPG_SINGLE))     /* not all algorithm variants */
    mode &= ~FPG_FIM16;         /* support a 16-items machine */
  e = eval & ~FPG_INVBXS;       /* remove flags from measure code */
  if ((target & ISR_RULES) || ((e > RE_NONE) && (e < RE_FNCNT)))
    mode &= ~FPG_FIM16;         /* restrict use of 16-items machine */
  if (!(target & ISR_MAXIMAL) || (mode & FPG_FIM16))
    mode &= ~FPG_TAIL;          /* tail pruning only for maximal */
  if (e <= RE_NONE)             /* if there is no evaluation, */
    prune = ITEM_MIN;           /* do not prune with evaluation */

  /* --- find frequent item sets --- */
  base = tbg_base(tabag);       /* get the underlying item base */
  if (!(target & ISR_RULES)     /* if to find plain item sets */
  &&  ((e <= RE_NONE) || (e >= RE_FNCNT))) {
    CLOCK(t);                   /* start the timer for the search */
    XMSG(stderr, "writing %s ... ", isr_name(report));
    r = fpgvars[algo](tabag, target, smin, mode, report);
    if (r < 0) return E_NOMEM;  /* search for frequent item sets */
    XMSG(stderr, "[%"SIZE_FMT" set(s)]", isr_repcnt(report));
    XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t)); }
  else {                        /* if rules or rule-based evaluation */
    CLOCK(t);                   /* start timer, print log message */
    XMSG(stderr, "finding frequent item set(s) ... ");
    a.istree = ist_create(base, IST_REVERSE, smin, body, conf);
    if (!a.istree) return cleanup(&a);
    zmin = isr_zmin(report);    /* create an item set tree */
    zmax = isr_zmax(report);    /* and configure it */
    if ((target & (ISR_CLOSED|ISR_MAXIMAL)) && (zmax < ITEM_MAX))
      zmax += 1;                /* adapt the maximum size */
    if (zmax > (m = tbg_max(tabag))) zmax = m;
    ist_setsize(a.istree, zmin, zmax);
    r = fpg_tree(tabag, smin, mode, a.istree);
    if (r < 0) return cleanup(&a); /* search for frequent item sets */
    XMSG(stderr, "done [%.2fs].\n", SEC_SINCE(t));
    if ((prune >  ITEM_MIN)     /* if to filter with evaluation */
    &&  (prune <= 0)) {         /* (backward and weak forward) */
      CLOCK(t);                 /* start the timer for filtering */
      XMSG(stderr, "filtering with evaluation ... ");
      ist_filter(a.istree, prune); /* mark non-qualifying item sets */
      XMSG(stderr, "done [%.2fs].\n", SEC_SINCE(t));
    }                           /* filter with evaluation */
    if (target & (ISR_CLOSED|ISR_MAXIMAL|ISR_GENERAS)) {
      CLOCK(t);                 /* start the timer for filtering */
      XMSG(stderr, "filtering for %s item sets ... ",
           (target & ISR_GENERAS) ? "generator" :
           (target & ISR_MAXIMAL) ? "maximal" : "closed");
      ist_clomax(a.istree, target | ((prune > ITEM_MIN) ? IST_SAFE :0));
      XMSG(stderr, "done [%.2fs].\n", SEC_SINCE(t));
    }                           /* filter closed/maximal/generators */
    CLOCK(t);                   /* start timer, print log message */
    XMSG(stderr, "writing %s ... ", isr_name(report));
    if (e != FPG_LDRATIO)       /* set additional evaluation measure */
      ist_seteval(a.istree, eval, agg, thresh, prune);
    ist_init(a.istree, order);  /* initialize the extraction */
    r = ist_report(a.istree, report, target);
    cleanup(&a);                /* report item sets/rules, */
    if (r < 0) return E_NOMEM;  /* then clean up the work memory */
    XMSG(stderr, "[%"SIZE_FMT" %s(s)]", isr_repcnt(report),
                 (target == ISR_RULES) ? "rule" : "set");
    XMSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  }                             /* print a log message */
  return 0;                     /* return 'ok' */
}  /* fpgrowth() */

/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/
#ifdef FPG_MAIN

static void help (void)
{                               /* --- print add. option information */
  #ifndef QUIET
  fprintf(stderr, "\n");        /* terminate startup message */
  printf("additional evaluation measures (option -e#)\n");
  printf("frequent item sets:\n");
  printf("  x   no measure (default)\n");
  printf("  b   binary logarithm of support quotient            (+)\n");
  printf("association rules:\n");
  printf("  x   no measure (default)\n");
  printf("  o   rule support (original def.: body & head)       (+)\n");
  printf("  c   rule confidence                                 (+)\n");
  printf("  d   absolute confidence difference to prior         (+)\n");
  printf("  l   lift value (confidence divided by prior)        (+)\n");
  printf("  a   absolute difference of lift value to 1          (+)\n");
  printf("  q   difference of lift quotient to 1                (+)\n");
  printf("  v   conviction (inverse lift for negated head)      (+)\n");
  printf("  e   absolute difference of conviction to 1          (+)\n");
  printf("  r   difference of conviction quotient to 1          (+)\n");
  printf("  k   conditional probability ratio                   (+)\n");
  printf("  j   importance (binary log. of cond. prob. ratio)   (+)\n");
  printf("  z   certainty factor (relative confidence change)   (+)\n");
  printf("  n   normalized chi^2 measure                        (+)\n");
  printf("  p   p-value from (unnormalized) chi^2 measure       (-)\n");
  printf("  y   normalized chi^2 measure with Yates' correction (+)\n");
  printf("  t   p-value from Yates-corrected chi^2 measure      (-)\n");
  printf("  i   information difference to prior                 (+)\n");
  printf("  g   p-value from G statistic/information difference (-)\n");
  printf("  f   Fisher's exact test (table probability)         (-)\n");
  printf("  h   Fisher's exact test (chi^2 measure)             (-)\n");
  printf("  m   Fisher's exact test (information gain)          (-)\n");
  printf("  s   Fisher's exact test (support)                   (-)\n");
  printf("All measures for association rules are also applicable\n");
  printf("to item sets and are then aggregated over all possible\n");
  printf("association rules with a single item in the consequent.\n");
  printf("The aggregation mode can be set with the option -a#.\n");
  printf("Measures marked with (+) must meet or exceed the threshold,\n");
  printf("measures marked with (-) must not exceed the threshold\n");
  printf("in order for the rule or item set to be reported.\n");
  printf("\n");
  printf("evaluation measure aggregation modes (option -a#)\n");
  printf("  x   no aggregation (use first value)\n");
  printf("  m   minimum of individual measure values\n");
  printf("  n   maximum of individual measure values\n");
  printf("  a   average of individual measure values\n");
  printf("\n");
  printf("fpgrowth algorithm variants (option -A#)\n");
  printf("  s   simple  tree nodes with only successor and parent\n");
  printf("  c   complex tree nodes with children and siblings "
               "(default)\n");
  printf("  d   top-down processing on a single prefix tree\n");
  printf("  t   top-down processing of the prefix trees\n");
  printf("Variant 'd' does not support mining closed/maximal item ");
  printf("sets,\nvariant 't' does not support the use of a k-items ");
  printf("machine, and\nonly variant 'c' supports item reordering ");
  printf("w.r.t. conditional support,\nbut closed/maximal item sets ");
  printf("can only be mined without reordering.\nThese restrictions ");
  printf("may be removed in future versions of this program.\n");
  printf("Rules as the target type or using a rule-based evaluation ");
  printf("measure\nenforces algorithm variant 'd'.\n");
  printf("information output format characters (option -v#)\n");
  printf("  %%%%  a percent sign\n");
  printf("  %%i  number of items (item set size)\n");
  printf("  %%a  absolute item set  support\n");
  printf("  %%s  relative item set  support as a fraction\n");
  printf("  %%S  relative item set  support as a percentage\n");
  printf("  %%b  absolute body set  support\n");
  printf("  %%x  relative body set  support as a fraction\n");
  printf("  %%X  relative body set  support as a percentage\n");
  printf("  %%h  absolute head item support\n");
  printf("  %%y  relative head item support as a fraction\n");
  printf("  %%Y  relative head item support as a percentage\n");
  printf("  %%c  rule confidence as a fraction\n");
  printf("  %%C  rule confidence as a percentage\n");
  printf("  %%l  lift value of a rule (confidence/prior)\n");
  printf("  %%L  lift value of a rule as a percentage\n");
  printf("  %%e  additional evaluation measure\n");
  printf("  %%E  additional evaluation measure as a percentage\n");
  printf("All format characters can be preceded by the number\n");
  printf("of significant digits to be printed (at most 32 digits),\n");
  printf("even though this value is ignored for integer numbers.\n");
  #endif                        /* print help information */
  exit(0);                      /* abort the program */
}  /* help() */

/*--------------------------------------------------------------------*/

static ITEM getbdr (char *s, char **end, double **border)
{                               /* --- get the support border */
  ITEM   i, k;                  /* loop variables */
  double *b;                    /* support border */

  assert(s && end && border);   /* check the function arguments */
  for (i = k = 0; s[i]; i++)    /* traverse the string and */
    if (s[i] == ':') k++;       /* count the number separators */
  *border = b = (double*)malloc((size_t)++k *sizeof(double));
  if (!b) return -1;            /* allocate a support border */
  for (i = 0; i < k; i++) {     /* traverse the parameters */
    b[i] = strtod(s, end);      /* get the next parameter and */
    if (*end == s) break;       /* check for an empty parameter */
    s = *end; if (*s++ != ':') break;
  }                             /* check for a colon */
  if (++i < k)                  /* shrink support array if possible */
    *border = (double*)realloc(b, (size_t)i *sizeof(double));
  return i;                     /* return number of support values */
}  /* getbdr() */

/*--------------------------------------------------------------------*/

static int setbdr (ISREPORT *report, SUPP w, ITEM zmin,
                   double **border, ITEM n)
{                               /* --- set the support border */
  double s;                     /* to traverse the support values */

  assert(report                 /* check the function arguments */
  &&    (w > 0) && (zmin >= 0) && border && (*border || (n <= 0)));
  while (--n >= 0) {            /* traverse the support values */
    s = (*border)[n];           /* transform to absolute count */
    s = ceilsupp((s >= 0) ? 0.01 *s *(double)w *(1-DBL_EPSILON) : -s);
    if (isr_setbdr(report, n+zmin, (RSUPP)s) < 0) return -1;
  }                             /* set support in item set reporter */
  if (*border) { free(*border); *border = NULL; }
  return 0;                     /* return 'ok' */
}  /* setbdr() */

/*--------------------------------------------------------------------*/

#ifndef NDEBUG                  /* if debug version */
  #undef  CLEANUP               /* clean up memory and close files */
  #define CLEANUP \
  if (twrite) twr_delete(twrite, 1); \
  if (report) isr_delete(report, 0); \
  if (tabag)  tbg_delete(tabag,  0); \
  if (tread)  trd_delete(tread,  1); \
  if (ibase)  ib_delete (ibase);     \
  if (border) free(border);
#endif

GENERROR(error, exit)           /* generic error reporting function */

/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{                               /* --- main function */
  int     i, k = 0;             /* loop variables, counters */
  char    *s;                   /* to traverse the options */
  CCHAR   **optarg = NULL;      /* option argument */
  CCHAR   *fn_inp  = NULL;      /* name of the input  file */
  CCHAR   *fn_out  = NULL;      /* name of the output file */
  CCHAR   *fn_sel  = NULL;      /* name of item selection file */
  CCHAR   *fn_psp  = NULL;      /* name of pattern spectrum file */
  CCHAR   *recseps = NULL;      /* record  separators */
  CCHAR   *fldseps = NULL;      /* field   separators */
  CCHAR   *blanks  = NULL;      /* blank   characters */
  CCHAR   *comment = NULL;      /* comment characters */
  CCHAR   *hdr     = "";        /* record header  for output */
  CCHAR   *sep     = " ";       /* item separator for output */
  CCHAR   *imp     = " <- ";    /* implication sign for ass. rules */
  CCHAR   *dflt    = " (%S)";   /* default format for check */
  CCHAR   *info    = dflt;      /* format for information output */
  int     target   = 's';       /* target type (e.g. closed/maximal) */
  ITEM    zmin     = 1;         /* minimum rule/item set size */
  ITEM    zmax     = ITEM_MAX;  /* maximum rule/item set size */
  double  supp     = 10;        /* minimum support of an item set */
  double  smax     = 100;       /* maximum support of an item set */
  SUPP    smin     = 1;         /* minimum support of an item set */
  SUPP    body     = 1;         /* minimum support of a rule body */
  int     orig     = 0;         /* flag for rule support definition */
  double  conf     = 80;        /* minimum confidence (in percent) */
  int     eval     = 'x';       /* additional evaluation measure */
  int     eflgs    = 0;         /* evaluation measure flags */
  int     agg      = 'x';       /* aggregation mode for eval. measure */
  double  thresh   = 10;        /* threshold for evaluation measure */
  ITEM    prune    = ITEM_MIN;  /* (min. size for) evaluation pruning */
  int     sort     = 2;         /* flag for item sorting and recoding */
  int     algo     = 'c';       /* variant of fpgrowth algorithm */
  int     mode     = FPG_DEFAULT;  /* search mode (e.g. pruning) */
  int     pack     = 16;        /* number of bit-packed items */
  int     mtar     = 0;         /* mode for transaction reading */
  int     scan     = 0;         /* flag for scanable item output */
  int     bdrcnt   = 0;         /* number of support values in border */
  int     stats    = 0;         /* flag for item set statistics */
  PATSPEC *psp;                 /* collected pattern spectrum */
  ITEM    m;                    /* number of items */
  TID     n;                    /* number of transactions */
  SUPP    w;                    /* total transaction weight */
  #ifndef QUIET                 /* if not quiet version */
  clock_t t;                    /* timer for measurements */

  prgname = argv[0];            /* get program name for error msgs. */

  /* --- print usage message --- */
  if (argc > 1) {               /* if arguments are given */
    fprintf(stderr, "%s - %s\n", argv[0], DESCRIPTION);
    fprintf(stderr, VERSION); } /* print a startup message */
  else {                        /* if no argument is given */
    printf("usage: %s [options] infile [outfile]\n", argv[0]);
    printf("%s\n", DESCRIPTION);
    printf("%s\n", VERSION);
    printf("-t#      target type                              "
                    "(default: %c)\n", target);
    printf("         (s: frequent, c: closed, m: maximal item sets,\n");
    printf("          g: generators, r: association rules)\n");
    printf("-m#      minimum number of items per set/rule     "
                    "(default: %"ITEM_FMT")\n", zmin);
    printf("-n#      maximum number of items per set/rule     "
                    "(default: no limit)\n");
    printf("-s#      minimum support of an item set/rule      "
                    "(default: %g%%)\n", supp);
    printf("-S#      maximum support of an item set/rule      "
                    "(default: %g%%)\n", smax);
    printf("         (positive: percentage, "
                     "negative: absolute number)\n");
    printf("-o       use original rule support definition     "
                    "(body & head)\n");
    printf("-c#      minimum confidence of an assoc. rule     "
                    "(default: %g%%)\n", conf);
    printf("-e#      additional evaluation measure            "
                    "(default: none)\n");
    printf("-a#      aggregation mode for evaluation measure  "
                    "(default: none)\n");
    printf("-d#      threshold for add. evaluation measure    "
                    "(default: %g%%)\n", thresh);
    printf("-z       invalidate eval. below expected support  "
                    "(default: evaluate all)\n");
    printf("-p#      (min. size for) pruning with evaluation  "
                    "(default: no pruning)\n");
    printf("         (< 0: weak forward, > 0 strong forward, "
                     "= 0: backward pruning)\n");
    printf("-q#      sort items w.r.t. their frequency        "
                    "(default: %d)\n", sort);
    printf("         (1: ascending, -1: descending, 0: do not sort,\n"
           "          2: ascending, -2: descending w.r.t. "
                    "transaction size sum)\n");
    printf("-A#      variant of the fpgrowth algorithm to use "
                    "(default: %c)\n", algo);
    printf("-x       do not prune with perfect extensions     "
                    "(default: prune)\n");
    printf("-l#      number of items for k-items machine      "
                    "(default: %d)\n", pack);
    printf("         (only for variants s and d, "
                    "options -As or -Ad)\n");
    printf("-i       do not sort items w.r.t. cond. support   "
                    "(default: sort)\n");
    printf("         (only for algorithm variant c, option -Ac)\n");
    printf("-u       do not use head union tail (hut) pruning "
                    "(default: use hut)\n");
    printf("         (only for maximal item sets, option -tm)\n");
    printf("-F#:#..  support border for filtering item sets   "
                    "(default: none)\n");
    printf("         (list of minimum support values, "
                    "one per item set size,\n");
    printf("         starting at the minimum size, "
                    "as given with option -m#)\n");
    printf("-R#      read item selection/appearance indicators\n");
    printf("-P#      write a pattern spectrum to a file\n");
    printf("-Z       print item set statistics "
                    "(number of item sets per size)\n");
    printf("-g       write item names in scanable form "
                    "(quote certain characters)\n");
    printf("-h#      record header  for output                "
                    "(default: \"%s\")\n", hdr);
    printf("-k#      item separator for output                "
                    "(default: \"%s\")\n", sep);
    printf("-I#      implication sign for association rules   "
                    "(default: \"%s\")\n", imp);
    printf("-v#      output format for set/rule information   "
                    "(default: \"%s\")\n", info);
    printf("-w       integer transaction weight in last field "
                    "(default: only items)\n");
    printf("-r#      record/transaction separators            "
                    "(default: \"\\n\")\n");
    printf("-f#      field /item        separators            "
                    "(default: \" \\t,\")\n");
    printf("-b#      blank   characters                       "
                    "(default: \" \\t\\r\")\n");
    printf("-C#      comment characters                       "
                    "(default: \"#\")\n");
    printf("-!       print additional option information\n");
    printf("infile   file to read transactions from           "
                    "[required]\n");
    printf("outfile  file to write item sets/assoc. rules to  "
                    "[optional]\n");
    return 0;                   /* print a usage message */
  }                             /* and abort the program */
  #endif  /* #ifndef QUIET */
  /* free option characters: jy [A-Z]\[ACFIPRSZ] */

  /* --- evaluate arguments --- */
  for (i = 1; i < argc; i++) {  /* traverse the arguments */
    s = argv[i];                /* get an option argument */
    if (optarg) { *optarg = s; optarg = NULL; continue; }
    if ((*s == '-') && *++s) {  /* -- if argument is an option */
      while (*s) {              /* traverse the options */
        switch (*s++) {         /* evaluate the options */
          case '!': help();                          break;
          case 't': target = (*s) ? *s++ : 's';      break;
          case 'm': zmin   = (ITEM)strtol(s, &s, 0); break;
          case 'n': zmax   = (ITEM)strtol(s, &s, 0); break;
          case 's': supp   =       strtod(s, &s);    break;
          case 'S': smax   =       strtod(s, &s);    break;
          case 'o': orig   = 1;                      break;
          case 'c': conf   =       strtod(s, &s);    break;
          case 'e': eval   = (*s) ? *s++ : 0;        break;
          case 'a': agg    = (*s) ? *s++ : 0;        break;
          case 'd': thresh =       strtod(s, &s);    break;
          case 'z': eflgs |= FPG_INVBXS;             break;
          case 'p': prune  = (ITEM)strtol(s, &s, 0); break;
          case 'q': sort   = (int) strtol(s, &s, 0); break;
          case 'A': algo   = (*s) ? *s++ : 0;        break;
          case 'x': mode  &= ~FPG_PERFECT;           break;
          case 'l': pack   = (int) strtol(s, &s, 0); break;
          case 'i': mode  &= ~FPG_REORDER;           break;
          case 'u': mode  &= ~FPG_TAIL;              break;
          case 'F': bdrcnt = getbdr(s, &s, &border); break;
          case 'R': optarg = &fn_sel;                break;
          case 'P': optarg = &fn_psp;                break;
          case 'Z': stats  = 1;                      break;
          case 'g': scan   = 1;                      break;
          case 'h': optarg = &hdr;                   break;
          case 'k': optarg = &sep;                   break;
          case 'I': optarg = &imp;                   break;
          case 'v': optarg = &info;                  break;
          case 'w': mtar  |= TA_WEIGHT;              break;
          case 'r': optarg = &recseps;               break;
          case 'f': optarg = &fldseps;               break;
          case 'b': optarg = &blanks;                break;
          case 'C': optarg = &comment;               break;
          default : error(E_OPTION, *--s);           break;
        }                       /* set the option variables */
        if (optarg && *s) { *optarg = s; optarg = NULL; break; }
      } }                       /* get an option argument */
    else {                      /* -- if argument is no option */
      switch (k++) {            /* evaluate non-options */
        case  0: fn_inp = s;      break;
        case  1: fn_out = s;      break;
        default: error(E_ARGCNT); break;
      }                         /* note filenames */
    }
  }
  if (optarg)       error(E_OPTARG);     /* check option arguments */
  if (k      < 1)   error(E_ARGCNT);     /* and number of arguments */
  if (zmin   < 0)   error(E_SIZE, zmin); /* check the size limits */
  if (zmax   < 0)   error(E_SIZE, zmax); /* and the minimum support */
  if (supp   > 100) error(E_SUPPORT, supp);
  if (bdrcnt < 0)   error(E_NOMEM);
  if ((conf  < 0) || (conf > 100))
    error(E_CONF, conf);        /* check the minimum confidence */
  if ((!fn_inp || !*fn_inp) && (fn_sel && !*fn_sel))
    error(E_STDIN);             /* stdin must not be used twice */
  switch (target) {             /* check and translate target type */
    case 's': target = ISR_ALL;              break;
    case 'c': target = ISR_CLOSED;           break;
    case 'm': target = ISR_MAXIMAL;          break;
    case 'g': target = ISR_GENERAS;          break;
    case 'r': target = ISR_RULES;            break;
    default : error(E_TARGET, (char)target); break;
  }                             /* (get target type code) */
  switch (eval) {               /* check and translate measure */
    case 'x': eval = RE_NONE;                break;
    case 'o': eval = RE_SUPP;                break;
    case 'c': eval = RE_CONF;                break;
    case 'd': eval = RE_CONFDIFF;            break;
    case 'l': eval = RE_LIFT;                break;
    case 'a': eval = RE_LIFTDIFF;            break;
    case 'q': eval = RE_LIFTQUOT;            break;
    case 'v': eval = RE_CVCT;                break;
    case 'e': eval = RE_CVCTDIFF;            break;
    case 'r': eval = RE_CVCTQUOT;            break;
    case 'k': eval = RE_CPROB;               break;
    case 'j': eval = RE_IMPORT;              break;
    case 'z': eval = RE_CERT;                break;
    case 'n': eval = RE_CHI2;                break;
    case 'p': eval = RE_CHI2PVAL;            break;
    case 'y': eval = RE_YATES;               break;
    case 't': eval = RE_YATESPVAL;           break;
    case 'i': eval = RE_INFO;                break;
    case 'g': eval = RE_INFOPVAL;            break;
    case 'f': eval = RE_FETPROB;             break;
    case 'h': eval = RE_FETCHI2;             break;
    case 'm': eval = RE_FETINFO;             break;
    case 's': eval = RE_FETSUPP;             break;
    case 'b': eval = FPG_LDRATIO;            break;
    default : error(E_MEASURE, (char)eval);  break;
  }  /* free: u w */            /* (get evaluation measure code) */
  eval |= eflgs;                /* add evaluation measure flags */
  switch (agg) {                /* check and translate agg. mode */
    case 'x': agg  = FPG_NONE;               break;
    case 'm': agg  = FPG_MIN;                break;
    case 'n': agg  = FPG_MAX;                break;
    case 'a': agg  = FPG_AVG;                break;
    default : error(E_AGGMODE, (char)agg);   break;
  }                             /* (get aggregation mode code) */
  switch (algo) {               /* check and translate alg. variant */
    case 's': algo = FPG_SIMPLE;             break;
    case 'c': algo = FPG_COMPLEX;            break;
    case 'd': algo = FPG_SINGLE;             break;
    case 't': algo = FPG_TOPDOWN;            break;
    default : error(E_VARIANT, (char)algo);  break;
  }                             /* (get fpgrowth algorithm code) */
  if (pack > 0)                 /* add packed items to search mode */
    mode |= (pack < 16) ? pack : 16;
  if (target & ISR_RULES) fn_psp = NULL;
  else conf = 100;              /* no pattern spectrum for rules */
  if (info == dflt) {           /* if default info. format is used, */
    if (target != ISR_RULES)    /* set default according to target */
         info = (supp < 0) ? " (%a)"     : " (%S)";
    else info = (supp < 0) ? " (%b, %C)" : " (%X, %C)";
  }                             /* select absolute/relative support */
  thresh *= 0.01;               /* scale the evaluation threshold */
  MSG(stderr, "\n");            /* terminate the startup message */

  /* --- read item selection/appearance indicators --- */
  ibase = ib_create(0, 0);      /* create an item base */
  if (!ibase) error(E_NOMEM);   /* to manage the items */
  tread = trd_create();         /* create a transaction reader */
  if (!tread) error(E_NOMEM);   /* and configure the characters */
  trd_allchs(tread, recseps, fldseps, blanks, "", comment);
  if (fn_sel) {                 /* if an item selection is given */
    CLOCK(t);                   /* start timer, open input file */
    if (trd_open(tread, NULL, fn_sel) != 0)
      error(E_FOPEN, trd_name(tread));
    MSG(stderr, "reading %s ... ", trd_name(tread));
    m = (target == ISR_RULES)   /* depending on the target type */
      ? ib_readapp(ibase,tread) /* read the item appearances */
      : ib_readsel(ibase,tread);/* or a simple item selection */
    if (m < 0) error((int)-m, ib_errmsg(ibase, NULL, 0));
    trd_close(tread);           /* close the input file */
    MSG(stderr, "[%"ITEM_FMT" item(s)]", ib_cnt(ibase));
    MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  }                             /* print a log message */

  /* --- read transaction database --- */
  tabag = tbg_create(ibase);    /* create a transaction bag */
  if (!tabag) error(E_NOMEM);   /* to store the transactions */
  CLOCK(t);                     /* start timer, open input file */
  if (trd_open(tread, NULL, fn_inp) != 0)
    error(E_FOPEN, trd_name(tread));
  MSG(stderr, "reading %s ... ", trd_name(tread));
  k = tbg_read(tabag, tread, mtar);
  if (k < 0) error(-k, tbg_errmsg(tabag, NULL, 0));
  trd_delete(tread, 1);         /* read the transaction database, */
  tread = NULL;                 /* then delete the table reader */
  m = ib_cnt(ibase);            /* get the number of items, */
  n = tbg_cnt(tabag);           /* the number of transactions, */
  w = tbg_wgt(tabag);           /* the total transaction weight */
  MSG(stderr, "[%"ITEM_FMT" item(s), %"TID_FMT, m, n);
  if (w != (SUPP)n) { MSG(stderr, "/%"SUPP_FMT, w); }
  MSG(stderr, " transaction(s)] done [%.2fs].", SEC_SINCE(t));
  if ((m <= 0) || (n <= 0))     /* check for at least one item */
    error(E_NOITEMS);           /* and at least one transaction */
  MSG(stderr, "\n");            /* terminate the log message */
  conf *= 0.01;                 /* scale the minimum confidence */
  supp = (supp >= 0) ? 0.01 *supp *(double)w *(1-DBL_EPSILON) : -supp;
  smax = (smax >= 0) ? 0.01 *smax *(double)w *(1+DBL_EPSILON) : -smax;
  body = (SUPP)ceilsupp(supp);  /* compute absolute support values */
  smin = (SUPP)ceilsupp((!(target & ISR_RULES) || orig)
       ? supp : ceilsupp(supp) *conf *(1-DBL_EPSILON));

  /* --- find frequent item sets/association rules --- */
  mode |= FPG_VERBOSE|FPG_NOCLEAN;
  k = fpg_data(tabag, target, smin, zmin, eval, algo, mode, sort);
  if (k) error(k);              /* prepare data for FP-growth */
  report = isr_create(ibase);   /* create an item set reporter */
  if (!report) error(E_NOMEM);  /* and configure it */
  isr_setsize(report,        zmin, zmax);
  isr_setsupp(report, (RSUPP)smin, (RSUPP)floorsupp(smax));
  if (setbdr(report, w, zmin, &border, bdrcnt) != 0)
    error(E_NOMEM);             /* set limits and support border */
  if (fn_psp && (isr_addpsp(report, NULL) < 0))
    error(E_NOMEM);             /* set a pattern spectrum if req. */
  if (isr_setfmt(report, scan, hdr, sep, imp, info) != 0)
    error(E_NOMEM);             /* set the output format strings */
  k = isr_open(report, NULL, fn_out);
  if (k) error(k, isr_name(report)); /* open the item set file */
  if ((fpg_repo(report, target, eval, thresh, algo, mode) < 0)
  ||  (isr_setup(report) < 0))  /* prepare reporter for FP-growth */
    error(E_NOMEM);             /* and set up the item set reporter */
  k = fpgrowth(tabag, target, smin, body, conf,
               eval, agg, thresh, prune, algo, mode, 0, report);
  if (k) error(k);              /* find frequent item sets */
  if (stats)                    /* print item set statistics */
    isr_prstats(report, stdout, 0);
  if (isr_close(report) != 0)   /* close the output file */
    error(E_FWRITE, isr_name(report));

  /* --- write pattern spectrum --- */
  if (fn_psp) {                 /* if to write a pattern spectrum */
    CLOCK(t);                   /* start timer, create table write */
    psp    = isr_getpsp(report);/* get the pattern spectrum */
    twrite = twr_create();      /* create a table writer and */
    if (!twrite) error(E_NOMEM);/* open the output file */
    if (twr_open(twrite, NULL, fn_psp) != 0)
      error(E_FOPEN,  twr_name(twrite));
    MSG(stderr, "writing %s ... ", twr_name(twrite));
    if (psp_report(psp, twrite, 1.0) != 0)
      error(E_FWRITE, twr_name(twrite));
    twr_delete(twrite, 1);      /* write the pattern spectrum */
    twrite = NULL;              /* and delete the table writer */
    MSG(stderr, "[%"SIZE_FMT" signature(s)]", psp_sigcnt(psp));
    MSG(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  }                             /* write a log message */

  /* --- clean up --- */
  CLEANUP;                      /* clean up memory and close files */
  SHOWMEM;                      /* show (final) memory usage */
  return 0;                     /* return 'ok' */
}  /* main() */

#endif  /* #ifdef FPG_MAIN ... */
