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
//  Class:      C++ Source
//  Filename:   SimUdpSocket.cpp
//  Author:     Brian Woodard
//  Purpose:    This module performs the following tasks:
//
//              See header file for details.
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "SimUdpSocket.h"

CSimUdpSocket::CSimUdpSocket()
{
   // initialize
   mIsOpen = false;
}

CSimUdpSocket::CSimUdpSocket(const char *IpAddress, int SendPort, int RecvPort)
{
   // initialize
   mIsOpen = false;

   // open the socket
   Open(IpAddress, SendPort, RecvPort);
}

CSimUdpSocket::~CSimUdpSocket()
{
   close(mSocket);
}

bool CSimUdpSocket::Open(const char *IpAddress, int SendPort, int RecvPort)
{
   int optval = 1;

   // check if already open
   if (mIsOpen)
      return true;

   strncpy(mIpAddress, IpAddress, sizeof(mIpAddress));
   mSendPort = SendPort;
   mReceivePort = RecvPort;

   // Initialize the output socket structure
   memset(&mAddressOut, 0, sizeof(mAddressOut));
   mAddressOut.sin_addr.s_addr = inet_addr(IpAddress);
   mAddressOut.sin_family = AF_INET;
   mAddressOut.sin_port = htons(SendPort);

   // Initialize the input socket structure
   memset(&mAddressIn, 0, sizeof(mAddressIn));
   mAddressIn.sin_addr.s_addr = INADDR_ANY;
   mAddressIn.sin_family = AF_INET;
   mAddressIn.sin_port = htons(RecvPort);

   // open the sockets
   if ((mSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      perror(" UDP Port Constructor: Output socket()");
      return false;
   }

   int bufSize = 65536;
   setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (char *)&bufSize, sizeof(int));
   setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, (char *)&bufSize, sizeof(int));

   // Set the socket options for broadcast address
   if (setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0)
   {
      perror(" UDP Port Constructor: SetSockOpt()");
      return false;
   }

   if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
   {
      perror(" UDP Port Constructor: SetSockOpt()");
      return false;
   }

   // include struct in_pktinfo in the message "ancilliary" control data
   setsockopt(mSocket, IPPROTO_IP, IP_PKTINFO, &optval, sizeof(optval));

   //  Bind to the  input socket
   if (bind(mSocket, (struct sockaddr *)&mAddressIn, sizeof(mAddressIn)) == -1)
   {
      perror("UDP Port Constructor(): bind()");
      return false;
   }

   mIsOpen = true;

   return true;
}

int CSimUdpSocket::SendToSocket(char *DataBuffer, int SizeInBytes)
{
   int send_status; /* Status of sendto function call */

   if (!mIsOpen)
      return 0;

   /* Send data out on the socket */
   send_status = sendto(mSocket, DataBuffer, SizeInBytes, 0, (struct sockaddr *)&mAddressOut, sizeof(mAddressOut));

   if (send_status < 0)
   {
      fprintf(stderr, "SendToSocket(): IP address %s, send port %d, receive port %d\n", mIpAddress, mSendPort, mReceivePort);
      perror("SendToSocket(): sendto()");
      return (-1);
   }

   return 0;
}

int CSimUdpSocket::ReceiveFromSocket(char *DataBuffer, int MaxSizeToRead)
{
   int bytes_returned;
   int fromlen;
   struct sockaddr insock;

   if (!mIsOpen)
      return 0;

   fromlen = MaxSizeToRead; /* This will get changed to actual number of bytes read in */

   /* Read any data received on the socket. */
   bytes_returned = recvfrom(mSocket, DataBuffer, MaxSizeToRead, 0, (struct sockaddr *)&insock, (socklen_t *)&fromlen);

   if (bytes_returned == -1)
   {
      if (errno == EAGAIN)
         return 0;
      else
      {
         return -1;
      }
   }

   return (bytes_returned);
}

int CSimUdpSocket::ReceiveFromSocket(char *DataBuffer, int MaxSizeToRead, char* FromIp, char* ToMcastIp)
{
   char               cmsgbuffer[4096];
   struct sockaddr_in addr_buffer;
   struct msghdr      msghdr;
   struct iovec       iov[1];
   int                bytes_returned;

   if (!mIsOpen)
      return 0;

   memset(iov, 0, sizeof(iov));
   memset(&msghdr, 0, sizeof(msghdr));

   iov[0].iov_base = DataBuffer;
   iov[0].iov_len = MaxSizeToRead;

   msghdr.msg_iov = iov;
   msghdr.msg_iovlen = 1;
   msghdr.msg_name = &addr_buffer;
   msghdr.msg_namelen = sizeof(addr_buffer);
   msghdr.msg_control = cmsgbuffer;
   msghdr.msg_controllen = sizeof(cmsgbuffer);
   msghdr.msg_flags = 0;

   bytes_returned = recvmsg(mSocket, &msghdr, 0);

   if (bytes_returned > 0)
   {
      // get some info from the msg
      strncpy(FromIp, inet_ntoa(addr_buffer.sin_addr), INET_ADDRSTRLEN);

      for ( // iterate through all the control headers
         struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msghdr);
         cmsg != NULL;
         cmsg = CMSG_NXTHDR(&msghdr, cmsg))
      {
         // ignore the control headers that don't match what we want
         if (cmsg->cmsg_level != IPPROTO_IP ||
            cmsg->cmsg_type != IP_PKTINFO)
         {
            continue;
         }
         struct in_pktinfo *pi = (struct in_pktinfo*)CMSG_DATA(cmsg);
         strncpy(ToMcastIp, inet_ntoa(pi->ipi_addr), INET_ADDRSTRLEN);
         // at this point, peeraddr is the source sockaddr
         // pi->ipi_spec_dst is the destination in_addr
         // pi->ipi_addr is the receiving interface in_addr
      }
   }

   return (bytes_returned);
}

void CSimUdpSocket::SetNonBlockingFlag()
{
   int socket_flags;

   // Get Socket flags
   socket_flags = fcntl(mSocket, F_GETFL);

   // Set non-blocking status
   socket_flags |= FNDELAY;

   // Re-set the socket flags
   fcntl(mSocket, F_SETFL, socket_flags);
}

void CSimUdpSocket::ClearNonBlockingFlag()
{
   int socket_flags;

   // Get Socket flags
   socket_flags = fcntl(mSocket, F_GETFL);

   // Set blocking status
   socket_flags &= ~FNDELAY;

   // Re-set the socket flags
   fcntl(mSocket, F_SETFL, socket_flags);
}

int CSimUdpSocket::SetTtl(unsigned char ttl)
{
   unsigned char theTtl = ttl;

   if (!mIsOpen)
      return -1;

   int status = setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_TTL, &theTtl, sizeof(theTtl));

   return status;
}

int CSimUdpSocket::SetMultiCast(const char *device_ip)
{
   struct in_addr interface_addr;

   if (!mIsOpen)
      return -1;

   interface_addr.s_addr = inet_addr(device_ip);

   int status = setsockopt(mSocket, IPPROTO_IP, IP_MULTICAST_IF, &interface_addr, sizeof(interface_addr));

   return status;
}

int CSimUdpSocket::JoinMcastGroup(const char *mcast_ip, const char *device_ip)
{
   struct ip_mreq mreq;

   if (!mIsOpen)
      return -1;

   mreq.imr_multiaddr.s_addr = inet_addr(mcast_ip);
   mreq.imr_interface.s_addr = inet_addr(device_ip);
   int status = setsockopt(mSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

   return status;
}

int CSimUdpSocket::DropMcastGroup(const char *mcast_ip, const char *device_ip)
{
   struct ip_mreq mreq;

   if (!mIsOpen)
      return -1;

   mreq.imr_multiaddr.s_addr = inet_addr(mcast_ip);
   mreq.imr_interface.s_addr = inet_addr(device_ip);
   int status = setsockopt(mSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));

   return status;
}

const char* CSimUdpSocket::GetCfgNameIpAddr(const char* CfgName)
{
   struct ifreq ifr;

   if (!mIsOpen) return nullptr;

   ifr.ifr_addr.sa_family = AF_INET;
   strncpy(ifr.ifr_name, CfgName, IFNAMSIZ-1);
   ioctl(mSocket, SIOCGIFADDR, &ifr);

   return inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
}
