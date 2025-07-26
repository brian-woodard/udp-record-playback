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
//  © Veraxx Engineering Corporation 2021.  All rights reserved.
//                                            
// DEVELOPED BY:
//  Veraxx Engineering Corporation                                           
//  14130 Sullyfield Circle Ste B
//  Chantilly, VA 20151
//  (703)880-9000 (Voice)                                                    
//  (703)880-9005 (Fax)                                                     
//
//-----------------------------------------------------------------------------
//
//  Title:      Print Data source
//  Class:      C++ source
//  Filename:   PrintData.cpp
//  Author:     Brian Woodard
//  Purpose:
//
//  Provides utility routines to print timestamp and print data.
//
//-----------------------------------------------------------------------------

#include "PrintData.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

char* CPrintData::mDataString = nullptr;
int   CPrintData::mDataLength = 0;

const char* CPrintData::GetDataAsString(char* Data, int Length)
{
   // format data received
   if (Length > 0)
   {
      char element[BYTES_PER_LINE+3] = { 0 };

      // allocate string long enough to hold data
      int length_required = ((Length + 8) / 8) * CHARS_PER_LINE;

      if (!mDataString)
      {
         mDataString = new char[length_required];
         mDataLength = length_required;
      }
      else if (mDataString && length_required > mDataLength)
      {
         delete [] mDataString;
         mDataString = new char[length_required];
         mDataLength = length_required;
      }

      GetTimeAsString();

      for (int i = 0; i < Length; i++)
      {
         if (i % BYTES_PER_LINE == 0)
         {
            if (i != 0)
            {
               // print data as string
               strcat(mDataString, "   (");
               memcpy(element, &Data[i-BYTES_PER_LINE], BYTES_PER_LINE);
               for (int j = 0; j < BYTES_PER_LINE; j++)
               {
                  // replace non-printable characters with '.'
                  if (element[j] >= 0 && element[j] <= 31)
                     element[j] = '.';
               }
               strcat(mDataString, element);
               strcat(mDataString, ")");
               memset(element, 0, sizeof(element));
            }
            sprintf(element, "\n%08x:", i);
            strcat(mDataString, element);
            memset(element, 0, sizeof(element));
         }
         if (i % 2 == 0)
         {
            // add a space every 2 bytes
            strcat(mDataString, " ");
         }

         // print data
         sprintf(element, "%02x", (unsigned char)Data[i]);
         strcat(mDataString, element);
         memset(element, 0, sizeof(element));
      }

      if (Length % BYTES_PER_LINE != 0)
      {
         // pad with spaces and print data as string
         for (int j = 0; j < ((BYTES_PER_LINE - (Length % BYTES_PER_LINE)) * 2) + ((BYTES_PER_LINE - (Length % BYTES_PER_LINE)) / 2); j++)
            strcat(mDataString, " ");
         strcat(mDataString, "   (");
         memcpy(element, &Data[Length-(Length % BYTES_PER_LINE)], Length % BYTES_PER_LINE);
         for (int j = 0; j < BYTES_PER_LINE; j++)
         {
            // replace non-printable characters with '.'
            if (element[j] >= 0 && element[j] <= 31)
               element[j] = '.';
         }
         strcat(mDataString, element);
         strcat(mDataString, ")");
         memset(element, 0, sizeof(element));
      }
      else
      {
         // print data as string
         strcat(mDataString, "   (");
         memcpy(element, &Data[Length-BYTES_PER_LINE], BYTES_PER_LINE);
         for (int j = 0; j < BYTES_PER_LINE; j++)
         {
            // replace non-printable characters with '.'
            if (element[j] >= 0 && element[j] <= 31)
               element[j] = '.';
         }
         strcat(mDataString, element);
         strcat(mDataString, ")");
         memset(element, 0, sizeof(element));
      }
   }

   return mDataString;
}

const char* CPrintData::GetTimeAsString()
{
   struct timeval tv;
   struct tm*     tm;

   if (!mDataString)
   {
      mDataString = new char[20];
      mDataLength = 20;
   }
   else if (mDataString && 20 > mDataLength)
   {
      delete [] mDataString;
      mDataString = new char[20];
      mDataLength = 20;
   }

   memset(mDataString, 0, mDataLength);

   gettimeofday(&tv, NULL);
   tm = localtime(&tv.tv_sec);

   sprintf(mDataString, "%02d:%02d:%02d.%06ld - ", tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec);

   return mDataString;
}
