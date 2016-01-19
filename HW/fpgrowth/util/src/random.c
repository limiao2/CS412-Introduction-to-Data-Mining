/*----------------------------------------------------------------------
  File    : random.c
  Contents: simple random number functions
  Author  : Christian Borgelt
  History : 2011.01.28 file created
            2012.01.09 functions brand() and xrand() added
            2012.01.10 function randn() added (normal distribution)
            2012.01.18 initialization function rseed() modified
            2012.01.24 NAN as indicator of empty buffer in nrand()
            2012.02.08 standard library functions removed from rseed()
            2013.09.12 optional fallback to functions from stdlib added
            2013.12.01 symmetric random number functions added
            2013.12.04 data type RNG added (random generator object)
----------------------------------------------------------------------*/
#include <limits.h>
#include <float.h>
#include <math.h>
#include "random.h"
#ifdef STORAGE
#include "storage.h"
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define RNORM       (1.0 /((double)UINT_MAX +1.0))

#ifdef _MSC_VER
#ifndef isnan
#define isnan(x)    _isnan(x)
#endif                          /* check for 'not a number' */
#endif                          /* MSC still does not support C99 */
#ifndef INFINITY
#define INFINITY    (DBL_MAX+DBL_MAX)
#endif
#ifndef NAN
#define NAN         (INFINITY-INFINITY)
#endif

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
#if !defined RAND_LIBC && !defined RAND_LIB48
static unsigned int rng[5] = {  /* registers for random number gen. */
  123456789, 362436069, 521288629, 88675123, 886756453 };
#endif
/*----------------------------------------------------------------------
  Random Number Functions
----------------------------------------------------------------------*/
#if defined RAND_LIB48          /* if to use library for rand48 */

extern void   srand48 (long int seed);
extern long   lrand48 (void);
extern double drand48 (void);

void          rseed (unsigned int seed)
{ srand48((long int)seed); }

unsigned int  urand (void)
{ return (unsigned int)lrand48(); }

double        drand (void)
{ return drand48(); }

double        xrand (void)
{ return drand48(); }

int           brand (void)
{ return (drand() >= 0.5) ? 1 : 0; }

/*--------------------------------------------------------------------*/
#elif defined RAND_LIBC         /* if to use basic functions */

void          rseed (unsigned int seed)
{ srand(seed); }

unsigned int  urand (void)
{ return (unsigned int)rand(); }

double        drand (void)
{ return (double)rand()/((double)RAND_MAX+1); }

double        xrand (void)
{ return (double)rand()/((double)RAND_MAX+1); }

int           brand (void)
{ return (rand() > RAND_MAX /2) ? 1 : 0; }

/*--------------------------------------------------------------------*/
#else                           /* if to use own functions */

void rseed (unsigned int seed)
{                               /* --- init. random number generator */
  rng[0] = seed;                /* use simple RNG to init. the buffer */
  rng[1] = seed = 69069 *seed +362437;
  rng[2] = seed = 69069 *seed +362437;
  rng[3] = seed = 69069 *seed +362437;
  rng[4] = seed = 69069 *seed +362437;
  urand(); urand(); urand(); urand(); urand();
}  /* rseed() */

/*--------------------------------------------------------------------*/

unsigned int urand (void)
{                               /* --- next random unsigned int */
  unsigned int t = (rng[0] ^ (rng[0] >> 7));
  rng[0] = rng[1]; rng[1] = rng[2];
  rng[2] = rng[3]; rng[3] = rng[4];
  rng[4] = (rng[4] ^ (rng[4] << 6)) ^ (t ^ (t << 13));
  return (rng[1] +rng[1] +1) *rng[4];
}  /* urand() */

/*----------------------------------------------------------------------
  Reference for this random number generator:
    Post by George Marsaglia (geo@stat.fsu.edu)
    Subject: Re: good C random number generator
    Newsgroup: comp.lang.c
    Date: 2003-05-13 08:55:05 PST
    http://groups.google.com/group/comp.lang.c/
      browse_thread/thread/a9915080a4424068/
----------------------------------------------------------------------*/

double drand (void)             /* --- next random double (simple) */
{ return urand() *RNORM; }

/*--------------------------------------------------------------------*/

double xrand (void)
{                               /* --- next random double (complex) */
  double x;                     /* buffer for random number */
  do { x = urand() *RNORM +urand() *(RNORM *RNORM);
  } while (x >= 1);             /* ensure that the number is < 1 */
  return x;                     /* return the generated number */
}  /* xrand() */

/*--------------------------------------------------------------------*/

int brand (void)                /* --- next random bit */
{ return (urand() > UINT_MAX /2) ? 1 : 0; }

#endif
/*--------------------------------------------------------------------*/

double randn (double rand(void))
{                               /* --- next N(0,1) distrib. number */
  static double b = NAN;        /* buffer for random number */
  double x, y, r;               /* coordinates and radius */

  if (!isnan(b)) {              /* if the buffer is full, */
    x = b; b = NAN; return x; } /* return the buffered number */
  do {                          /* pick a random point */
    x = 2 *rand() -1;           /* in the unit square [-1,1]^2 */
    y = 2 *rand() -1;           /* and check whether it lies */
    r = x*x +y*y;               /* inside the unit circle */
  } while ((r > 1) || (r == 0));
  r = sqrt(-2*log(r)/r);        /* factor for Box-Muller transform */
  b = x *r;                     /* save one of the random numbers */
  return y *r;                  /* and return the other */
}  /* randn() */

/*----------------------------------------------------------------------
Source for the polar method to generate normally distributed numbers
(also known as Box-Muller transform):
  D.E. Knuth.
  The Art of Computer Programming, Vol. 2: Seminumerical Algorithms
  Addison-Wesley, Reading, MA, USA 1998
  pp. 122-123
----------------------------------------------------------------------*/

RNGFN *rng_tab[] = {            /* table of random functions */
  /* RNG_UNIFORM/RNG_RECT  0 */ rng_rect,
  /* RNG_TRIANG            1 */ rng_triang,
  /* RNG_GAUSS/RNG_NORMAL  2 */ rng_normal,
};

/*----------------------------------------------------------------------
  (Pseudo-)Random Number Generator Functions
----------------------------------------------------------------------*/

RNG* rng_create (unsigned int seed)
{                               /* --- create random number generator */
  RNG *rng;                     /* created random number generator */

  rng = (RNG*)malloc(sizeof(RNG));
  if (!rng) return NULL;        /* create a random number generator */
  rng_seed(rng, seed);          /* and initialize it */
  rng->type  = RNG_UNIFORM;     /* set default values for type */
  rng->sigma = 1.0;             /* and density function parameter */
  return rng;                   /* return the created generator */
}  /* rng_create() */

/*--------------------------------------------------------------------*/

RNG* rng_createx (unsigned int seed, int type, double sigma)
{                               /* --- create random number generator */
  RNG *rng;                     /* created random number generator */

  rng = (RNG*)malloc(sizeof(RNG));
  if (!rng) return NULL;        /* create a random number generator */
  rng_seed(rng, seed);          /* and initialize it */
  if ((type < 0) || (type > (int)(sizeof(rng_tab)/sizeof(rng_tab[0]))))
    type = RNG_UNIFORM;         /* ensure a proper type */
  rng->type  = type;            /* store the parameters */
  rng->sigma = (sigma <= 0) ? 0 : sigma;
  return rng;                   /* return the created generator */
}  /* rng_createx() */

/*--------------------------------------------------------------------*/

void rng_seed (RNG *rng, unsigned int seed)
{                               /* -- init. random number generator */
  rng->state[0] = seed;         /* use simple RNG to init. the buffer */
  rng->state[1] = seed = 69069 *seed +362437;
  rng->state[2] = seed = 69069 *seed +362437;
  rng->state[3] = seed = 69069 *seed +362437;
  rng->state[4] = seed = 69069 *seed +362437;
  rng_uint(rng); rng_uint(rng); rng_uint(rng);
  rng_uint(rng); rng_uint(rng); /* skip some initial values */
  rng->b = NAN;                 /* invalidate the buffer */
}  /* rng_seed() */

/*--------------------------------------------------------------------*/

void rng_seedx (RNG *rng,       unsigned int a,
                unsigned int b, unsigned int c,
                unsigned int d, unsigned int e)
{                               /* -- init. random number generator */
  rng->state[0] = a; rng->state[1] = b; rng->state[2] = c;
  rng->state[3] = d; rng->state[4] = e;/* store init. values */
  rng->b = NAN;                 /* and invalidate the buffer */
}  /* rng_seed() */

/*--------------------------------------------------------------------*/

unsigned int rng_uint (RNG *rng)
{                               /* --- generate random unsigned int */
  unsigned int t = (rng->state[0] ^ (rng->state[0] >> 7));
  rng->state[0] = rng->state[1]; rng->state[1] = rng->state[2];
  rng->state[2] = rng->state[3]; rng->state[3] = rng->state[4];
  rng->state[4] = (rng->state[4] ^ (rng->state[4] << 6))
                ^ (t ^ (t << 13));
  return (rng->state[1] +rng->state[1] +1) *rng->state[4];
}  /* rng_uint() */

/*--------------------------------------------------------------------*/

double rng_dbl (RNG *rng)
{ return rng_uint(rng) *RNORM; }

/*--------------------------------------------------------------------*/

double rng_dblx (RNG *rng)
{                               /* --- random double with full prec. */
  double x;                     /* buffer for random number */
  do { x = rng_uint(rng) *RNORM +rng_uint(rng) *(RNORM *RNORM);
  } while (x >= 1);             /* ensure that the number is < 1 */
  return x;                     /* return the generated number */
}  /* rng_dblx() */

/*--------------------------------------------------------------------*/

int rng_bit (RNG *rng)
{ return (rng_uint(rng) > UINT_MAX /2) ? 1 : 0; }

/*--------------------------------------------------------------------*/

double rng_norm (RNG *rng)
{                               /* --- generate std. normal value */
  double x, y, r;               /* coordinates and radius */

  if (!isnan(rng->b)) {         /* if the buffer is full, return it */
    x = rng->b; rng->b = NAN; return x; }
  do {                          /* pick a random point */
    x = 2 *rng_dbl(rng) -1;     /* in the unit square [-1,1]^2 */
    y = 2 *rng_dbl(rng) -1;     /* and check whether it lies */
    r = x*x +y*y;               /* inside the unit circle */
  } while ((r > 1) || (r == 0));
  r = sqrt(-2*log(r)/r);        /* factor for Box-Muller transform */
  rng->b = x *r;                /* save one of the random numbers */
  return y *r;                  /* and return the other */
}  /* rng_norm() */

/*----------------------------------------------------------------------
  Symmetric Density (Pseudo-)Random Number Generator Functions
----------------------------------------------------------------------*/

double rng_rect (RNG *rng)
{ return (rng->sigma > 0) ? (rng_dbl(rng)-0.5) *2*rng->sigma : 0;}

/*--------------------------------------------------------------------*/

double rng_triang (RNG *rng)
{                               /* --- triangular density */
  double y;                     /* sample from uniform density */
  if (rng->sigma <= 0) return 0;/* return zero if sigma vanishes */
  y = rng_dbl(rng);             /* transform from uniform density */
  return rng->sigma *((y < 0.5) ? sqrt(2*y)-1.0 : 1.0-sqrt(2*(1.0-y)));
}  /* rng_triang() */

/*--------------------------------------------------------------------*/

double rng_normal (RNG *rng)
{ return (rng->sigma > 0) ? rng_norm(rng) *rng->sigma : 0; }
