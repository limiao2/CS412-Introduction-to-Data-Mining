/*----------------------------------------------------------------------
  File    : sigint.c
  Contents: handling interrupt signals/execution abortion
  Author  : Christian Borgelt
  History : 2015.03.04 file created
----------------------------------------------------------------------*/
#ifdef _WIN32                   /* if MS Windows system */
#include <windows.h>
#else                           /* if Linux/Unix system */
#define _POSIX_C_SOURCE 200809L /* needed for sigaction */
#endif
#ifndef __SIGINT__
#define __SIGINT__
#include <signal.h>
#include "sigint.h"

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
static volatile sig_atomic_t aborted = 0;
                                /* whether abort interrupt received */
#ifndef _WIN32
static struct sigaction sigold; /* old signal action (to restore) */
static struct sigaction signew; /* new signal action (this module) */
#endif

/*----------------------------------------------------------------------
  External Functions
----------------------------------------------------------------------*/
#ifdef MATLAB_MEX_FILE
extern int utIsInterruptPending(); /* undocumented Matlab function */
#endif
/*----------------------------------------------------------------------
References:
  http://www.caam.rice.edu/~wy1/links/mex_ctrl_c_trick/
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
#ifdef _WIN32

static BOOL WINAPI sighandler (DWORD type)
{ if (type == SIGINT) sig_abort(-1); return TRUE; }

/*--------------------------------------------------------------------*/

void sig_install (void)         /* --- install signal handler */
{ SetConsoleCtrlHandler(sighandler, TRUE); }

/*--------------------------------------------------------------------*/

void sig_remove (void)          /* --- remove signal handler */
{ SetConsoleCtrlHandler(sighandler, FALSE); }

/*--------------------------------------------------------------------*/
#else                           /* --- #ifndef _WIN32 */

static void sighandler (int type)
{ if (type == SIGINT) sig_abort(-1); }

/*--------------------------------------------------------------------*/

void sig_install (void)
{                               /* --- install signal handler */
  signew.sa_handler = sighandler;
  signew.sa_flags   = 0;
  sigemptyset(&signew.sa_mask);
  sigaction(SIGINT, &signew, &sigold);
}  /* siginstall() */

/*--------------------------------------------------------------------*/

void sig_remove (void)          /* --- remove signal handler */
{ sigaction(SIGINT, &sigold, (struct sigaction*)0); }

#endif
/*--------------------------------------------------------------------*/

void sig_abort (int state)
{ aborted = state; }            /* --- set the abort state */

/*--------------------------------------------------------------------*/

int sig_aborted (void)
{                               /* --- check the abort state */
  #ifdef MATLAB_MEX_FILE
  if (utIsInterruptPending()) sig_abort(-1);
  #endif
  return aborted;
}

#endif
