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
//  Class:      C++ Source
//  Filename:   SimTimer.cpp
//  $Revision: 1.1 $
//  Author:     Rob Milchenski
//  Purpose:    This module performs the following tasks:
//
//              This file contains the CSimTimer class source.
//
//--------------------------------------------------------------------
//  Change History is automatically maintained -- DO NOT EDIT
//--------------------------------------------------------------------
//
//  Change History:
//
//      $Log: SimTimer.cpp,v $
//      Revision 1.1  2011-11-03 18:18:48-04  gilroy
//      Initial revision
//
//      Revision 1.1  2010-10-28 11:45:22-04  gilroy
//      Initial revision
//
//      Revision 1.1  2009-11-18 14:21:30-05  gilroy
//      Initial revision
//
//      Revision 1.1  2007-01-15 16:36:58-05  rob
//      Initial revision
//
//      Revision 1.1  2004-08-18 18:11:28-04  rob
//      Initial revision
//
//
//
//
//----------------------------------------------------------------------

#include "SimTimer.h"
#include <stdio.h>

CSimTimer::CSimTimer()
{
   mState = STOPPED;
}

CSimTimer::~CSimTimer()
{
}

void CSimTimer::Start(float TimeSec)    
{
   mTimerDuration = TimeSec;
   mStartTime = std::chrono::high_resolution_clock::now();
   mState = RUNNING;
}

eTimerState CSimTimer::Check(void)
{
   if (mState == RUNNING)
   {
      if (TimeElapsed() >= mTimerDuration)
         mState = STOPPED;
   }
   return mState;
}

float CSimTimer::TimeElapsed(void)
{
   if (mState == STOPPED)
   {
      return mTimerDuration;
   }
   else
   {
      auto elapsed = std::chrono::duration_cast<float_secs>(std::chrono::high_resolution_clock::now() - mStartTime);
      return elapsed.count();
   }
}

void CSimTimer::GetCurrentTimeStr(char* TimeStr)
{
   struct timeval tv;
   struct tm*     tm;
   time_t         tm_secs;

   // get timestamp
   gettimeofday(&tv, NULL);
   tm_secs = tv.tv_sec;
   tm = localtime(&tm_secs);
   sprintf(TimeStr, "%02d:%02d:%02d.%06d", tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec);
}

double CSimTimer::GetCurrentTime()
{
   struct timespec tm;

   clock_gettime(CLOCK_MONOTONIC, &tm);
   return (double)tm.tv_sec + (double)tm.tv_nsec / 1000000000.0;
}
