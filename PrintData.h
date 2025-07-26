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
//  Title:      Print Data header
//  Class:      C++ header
//  Filename:   PrintData.h
//  Author:     Brian Woodard
//  Purpose:
//
//  Provides utility routines to print data as string.
//
//-----------------------------------------------------------------------------

#pragma once

class CPrintData
{
public:

   static constexpr int BYTES_PER_LINE = 8;
   static constexpr int CHARS_PER_LINE = 50;

   CPrintData() {}
   ~CPrintData() = default;

   static const char* GetDataAsString(char* Data, int Length);
   static const char* GetTimeAsString();

private:

   static char* mDataString;
   static int   mDataLength;

};
