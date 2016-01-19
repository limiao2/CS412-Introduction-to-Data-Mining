/*----------------------------------------------------------------------
  File    : random.h
  Contents: simply random number functions
  Author  : Christian Borgelt
  History : 2011.01.28 file created
            2012.01.09 functions brand() and frand() added
            2012.01.10 function randn() added (normal distribution)
            2013.12.01 symmetric random number functions added
            2013.12.04 data type RNG added (random generator object)
----------------------------------------------------------------------*/
#ifndef __RANDOM__
#define __RANDOM__
#include <stdlib.h>

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define RNG_UNIFORM     0       /* uniform     density function */
#define RNG_RECT        0       /* rectangular density function */
#define RNG_TRIANG      1       /* triangular  density function */
#define RNG_GAUSS       2       /* Gaussian    density function */
#define RNG_NORMAL      2       /* normal      density function */

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- random number generator --- */
  unsigned int state[5];        /* registers for generator state */
  double       b;               /* buffer for Box-Muller transform */
  int          type;            /* density type (e.g. RNG_RECT) */
  double       sigma;           /* std. dev. or width parameter */
} RNG;                          /* (random number generator) */

typedef double RNGFN (RNG *rng);

/*----------------------------------------------------------------------
  Constants
----------------------------------------------------------------------*/
extern RNGFN *rng_tab[];        /* table of random number functions */

/*----------------------------------------------------------------------
  Random Number Functions
----------------------------------------------------------------------*/
extern void         rseed       (unsigned int seed);
extern unsigned int urand       (void);
extern double       drand       (void);
extern double       xrand       (void);
extern int          brand       (void);
extern double       randn       (double rand (void));

extern RNG*         rng_create  (unsigned int seed);
extern void         rng_delete  (RNG *rng);
extern void         rng_seed    (RNG *rng, unsigned int seed);
extern void         rng_seedx   (RNG *rng,       unsigned int a,
                                 unsigned int b, unsigned int c,
                                 unsigned int d, unsigned int e);
extern unsigned int rng_uint    (RNG *rng);
extern double       rng_dbl     (RNG *rng);
extern double       rng_dblx    (RNG *rng);
extern int          rng_bit     (RNG *rng);
extern double       rng_norm    (RNG *rng);

extern RNG*         rng_createx (unsigned int seed,
                                 int type, double sigma);
extern int          rng_type    (RNG *rng);
extern double       rng_sigma   (RNG *rng);
extern double       rng_next    (RNG *rng);
extern double       rng_uniform (RNG *rng);
extern double       rng_rect    (RNG *rng);
extern double       rng_triang  (RNG *rng);
extern double       rng_gauss   (RNG *rng);
extern double       rng_normal  (RNG *rng);

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define rng_delete(g)   free(g)
#define rng_type(g)     ((g)->type)
#define rng_sigma(g)    ((g)->sigma)
#define rng_next(g)     (rng_tab[(g)->type](g))
#define rng_uniform(g)  rng_rect(g)
#define rng_gauss(g)    rng_normal(g)

#endif
