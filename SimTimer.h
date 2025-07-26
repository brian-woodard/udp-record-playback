//-----------------------------------------------------------------------------
//                            UNCLASSIFIED
//-----------------------------------------------------------------------------
//                    DO NOT REMOVE OR MODIFY THIS HEADER
//-----------------------------------------------------------------------------
//
//  This software and the accompanying documentation are provided to the U.S.
//  Government with unlimited rights as provided in DFARS § 252.227-7014.  The
//  contractor, Veraxx Engineering Corporation, retains ownership, the
//  copyrights, and all other rights.
//
//  © Veraxx Engineering Corporation 2018.  All rights reserved.
//
// DEVELOPED BY:
//  Veraxx Engineering Corporation
//  14130 Sullyfield Circle, Suite B
//  Chantilly, VA 20151
//  (703)880-9000 (Voice)
//  (703)880-9005 (Fax)
//-----------------------------------------------------------------------------
//  Title:      Sim Timer
//  Class:      C++ Header
//  Filename:   SimTimer.h
//  $Revision: 1.1 $
//  Author:     Rob Milchenski
//  Purpose:    This module performs the following tasks:
//
//              This file contains the Sim Timer class definitions.
//              Use this class to create an instance of a count down timer.
//              Call Tick to progress the timer, Start to begin a count down
//              and Kill to stop a timer.
//
//--------------------------------------------------------------------
//  Change History is automatically maintained -- DO NOT EDIT
//--------------------------------------------------------------------
//
//  Change History:
//
//      $Log: SimTimer.h,v $
//      Revision 1.1  2011-11-03 18:18:51-04  gilroy
//      Initial revision
//
//      Revision 1.1  2010-10-28 11:45:26-04  gilroy
//      Initial revision
//
//      Revision 1.1  2009-11-18 14:21:17-05  gilroy
//      Initial revision
//
//      Revision 1.1  2007-01-15 16:36:59-05  rob
//      Initial revision
//
//      Revision 1.1  2004-08-18 18:11:28-04  rob
//      Initial revision
//
//
//
//----------------------------------------------------------------------

#ifndef SIM_TIMER_H
#define SIM_TIMER_H

#include <chrono>
#include <time.h>
#include <sys/time.h>

enum eTimerState {STOPPED, RUNNING};

class CSimTimer
{
   public:

   typedef std::chrono::duration<float> float_secs;

   CSimTimer();
   ~CSimTimer();

   void Start(float TimeSec);                   // Start a timers count down
   
   eTimerState Check(void);                     // Check the state of a timer
      
   float TimeElapsed(void);                     // Determine the time elapsed on a timer
   
   void  Kill(void) {mState=STOPPED;}           // Kill a timer

   static void GetCurrentTimeStr(char*);
   static double GetCurrentTime();

private:

   /* Timer variables */
   eTimerState  mState;                                       //   The current state of the timer.
   std::chrono::high_resolution_clock::time_point mStartTime; //   The time the timer was started.
   float mTimerDuration;                                      //   The duration the timer will run.
};

#endif   // ifndef TIMER_H
