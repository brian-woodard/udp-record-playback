//-----------------------------------------------------------------------------
//                               UNCLASSIFIED
//-----------------------------------------------------------------------------
//                    DO NOT REMOVE OR MODIFY THIS HEADER
//-----------------------------------------------------------------------------
//  This software and the accompanying documentation are provided to the U.S.
//  Government with unlimited rights as provided in DFARS section 252.227-7014.
//  The contractor, Veraxx Engineering Corporation, retains ownership, the
//  copyrights, and all other rights.
//
//  Copyright Veraxx Engineering Corporation 2023.  All rights reserved.
//
// DEVELOPED BY:
//  Veraxx Engineering Corporation
//  14130 Sullyfield Circle Ste. B
//  Chantilly, VA 20151
//  (703)880-9000 (Voice)
//  (703)880-9005 (Fax)
//-----------------------------------------------------------------------------
//  Title:      SimUdpSocket CSU
//  Class:      C++ Header
//  Filename:   SimUdpSocket.h
//  Author:     Brian Woodard
//  Purpose:    This module performs the following tasks:
//
//! \class CSimUdpSocket
//! \brief UDP Ethernet socket class
//!
//! Provides functionality to open a UDP Ethernet socket and read/write data.
//!
//
//------------------------------------------------------------------------------

#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>

class CSimUdpSocket
{
public:
   CSimUdpSocket();
   CSimUdpSocket(const char *IpAddr, int SendPort, int ReceivePort);
   ~CSimUdpSocket();

   bool Open(const char *IpAddr, int SendPort, int ReceivePort);

   int SendToSocket(char *DataBuffer, int SizeInBytes);
   int ReceiveFromSocket(char *DataBuffer, int MaxSizeToRead);
   int ReceiveFromSocket(char *DataBuffer, int MaxSizeToRead, char* FromIp, char* ToMcastIp);

   void SetNonBlockingFlag();
   void ClearNonBlockingFlag();

   bool IsOpen() const { return mIsOpen; }

   int SetTtl(unsigned char ttl);
   int SetMultiCast(const char *device_ip);
   int JoinMcastGroup(const char *mcast_ip, const char *device_ip);
   int DropMcastGroup(const char *mcast_ip, const char *device_ip);
   const char* GetCfgNameIpAddr(const char* CfgName);

private:
   bool mIsOpen;
   int mSocket;
   char mIpAddress[50];
   int mSendPort;
   int mReceivePort;

   struct sockaddr_in mAddressOut;
   struct sockaddr_in mAddressIn;
};
