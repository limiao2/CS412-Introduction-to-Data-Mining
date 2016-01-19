/*----------------------------------------------------------------------
  File    : sigint.h
  Contents: handling interrupt signals/execution abortion
  Author  : Christian Borgelt
  History : 2015.03.04 file created
----------------------------------------------------------------------*/
#ifndef __SIGINTX__
#define __SIGINTX__

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern void sig_install (void);
extern void sig_remove  (void);
extern void sig_abort   (int state);
extern int  sig_aborted (void);

#endif
