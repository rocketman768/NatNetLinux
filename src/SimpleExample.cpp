/*
 * SimpleExample.cpp is part of NatNetLinux, and is Copyright 2013,
 * Philip G. Lee <rocketman768@gmail.com>
 *
 * NatNetLinux is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NatNetLinux is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NatNetLinux.  If not, see <http://www.gnu.org/licenses/>.
 */
    
#define MULTICAST_ADDRESS "239.255.42.99"
#define PORT_COMMAND      1510
#define PORT_DATA         1511

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <NatNetLinux/NatNet.h>
#include <NatNetLinux/CommandListener.h>
#include <NatNetLinux/FrameListener.h>

void helpAndExit()
{
   std::cout
      << "Usage:" << std::endl
      << "   simple-example [local address] [server address]" << std::endl
      << "   local address  - Local interface IPv4 address, e.g. 192.168.0.2" << std::endl
      << "   server address - Server IPv4 address, e.g. 192.168.0.3" << std::endl;
   exit(1);
}

/*!
 * \brief Creates a socket for communicating commands.
 * 
 * \param inAddr our local address
 * \returns socket descriptor bound to PORT_COMMAND and local address
 */
int createCommandSocket( uint32_t inAddr )
{
   // Asking for a buffer of 1MB = 2^20 bytes. This is what NP does, but this
   // seems far too large on Linux systems where the max is usually something
   // like 256 kB.
   const int rcvBufSize = 0x100000;
   int sd;
   int tmp;
   socklen_t len;
   struct sockaddr_in sockAddr;
   
   sd = socket(AF_INET, SOCK_DGRAM, 0);
   if( sd < 0 )
   {
      std::cerr << "Could not open socket. Error: " << errno << std::endl;
      exit(1);
   }
   
   // Bind socket
   memset(&sockAddr, 0, sizeof(sockAddr));
   sockAddr.sin_family = AF_INET;
   sockAddr.sin_port = htons(PORT_COMMAND);
   //sockAddr.sin_port = htons(0);
   sockAddr.sin_addr.s_addr = inAddr;
   tmp = bind( sd, (struct sockaddr*)&sockAddr, sizeof(sockAddr) );
   if( tmp < 0 )
   {
      std::cerr << "Could not bind socket. Error: " << errno << std::endl;
      close(sd);
      exit(1);
   }
   
   int value = 1;
   tmp = setsockopt( sd, SOL_SOCKET, SO_BROADCAST, (char*)&value, sizeof(value) );
   if( tmp < 0 )
   {
      std::cerr << "Could not set socket to broadcast mode. Error: " << errno << std::endl;
      close(sd);
      exit(1);
   }
   
   setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char*)&rcvBufSize, sizeof(rcvBufSize));
   getsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char*)&tmp, &len);
   if( tmp != rcvBufSize )
   {
      std::cerr << "Could not set receive buffer size. Asked for "
         << rcvBufSize << "B got " << tmp << "B" << std::endl;
      //close(sd);
      //exit(1);
   }
   
   return sd;
}

/*!
 * \brief Creates a socket to read data from the server.
 * 
 * The socket returned from this function is bound to \c PORT_DATA and
 * \c INADDR_ANY, and is added to the multicast group given by
 * \c MULTICAST_ADDRESS.
 * 
 * \param inAddr our local address
 * \returns socket bound as described above
 */
int createDataSocket( uint32_t inAddr )
{
   int sd;
   int value;
   int tmp;
   struct sockaddr_in localSock;
   struct ip_mreq group;
   
   sd = socket(AF_INET, SOCK_DGRAM, 0);
   value = 1;
   tmp = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));
   if( tmp < 0 )
   {
      std::cerr << "ERROR: Could not set socket option." << std::endl;
      close(sd);
      return -1;
   }
   
    // Bind the socket to a port.
   memset((char*)&localSock, 0, sizeof(localSock));
   localSock.sin_family = AF_INET;
   localSock.sin_port = htons(PORT_DATA);
   localSock.sin_addr.s_addr = INADDR_ANY;
   bind(sd, (struct sockaddr*)&localSock, sizeof(localSock));
   
   // Connect a local interface address to the multicast interface address.
   group.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDRESS);
   group.imr_interface.s_addr = inAddr;
   tmp = setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&group, sizeof(group));
   if( tmp < 0 )
   {
      std::cerr << "ERROR: Could not add the interface to the multicast group." << std::endl;
      close(sd);
      return -1;
   }
   
   return sd;
}

//! \brief Convert internet address to string.
void addrToStr( char* str, size_t len, const struct in_addr addr )
{
   const char* s = inet_ntoa( addr );
   strncpy( str, s, len );
}

void readOpts( uint32_t& localAddress, uint32_t& serverAddress, int argc, char* argv[] )
{
   if( argc > 2 )
   {
      localAddress = inet_addr(argv[1]);
      serverAddress = inet_addr(argv[2]);
   }
   else
      helpAndExit();
}

int main(int argc, char* argv[])
{
   unsigned char natNetMajor;
   unsigned char natNetMinor;
   
   int sdCommand;
   int sdData;
   
   uint32_t localAddress;
   uint32_t serverAddress;
   
   readOpts( localAddress, serverAddress, argc, argv );
   
   // Use this socket address to send commands to the server.
   struct sockaddr_in serverCommands;
   memset(&serverCommands, 0, sizeof(serverCommands));
   serverCommands.sin_family = AF_INET;
   serverCommands.sin_port = htons(PORT_COMMAND);
   serverCommands.sin_addr.s_addr = serverAddress;
   
   // Use this socket address to receive data from the server.
   struct sockaddr_in serverData;
   memset(&serverData, 0, sizeof(serverData));
   serverData.sin_family = AF_INET;
   serverData.sin_port = htons(PORT_DATA);
   serverData.sin_addr.s_addr = serverAddress;
   
   sdCommand = createCommandSocket( localAddress );
   sdData = createDataSocket( localAddress );
   
   CommandListener commandListener(sdCommand);
   commandListener.start();

   // Send a ping packet to the server so that it sends us the NatNet version
   // in its response to commandListener.
   NatNetPacket ping = NatNetPacket::pingPacket();
   ping.send(sdCommand, serverCommands);
   
   // Wait here for ping response to give us the NatNet version.
   commandListener.getNatNetVersion(natNetMajor, natNetMinor);
   std::cout << "Main thread got version " << static_cast<int>(natNetMajor) << "." << static_cast<int>(natNetMinor) << std::endl;
   
   FrameListener frameListener(sdData, natNetMajor, natNetMinor);
   frameListener.start();
   
   // Wait for listener threads to finish. Probably never. Just CTRL-C.
   frameListener.join();
   commandListener.join();
   
   // Epilogue
   close(sdData);
   close(sdCommand);
   return 0;
}
