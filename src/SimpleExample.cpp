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
   
   sdCommand = NatNet::createCommandSocket( localAddress );
   sdData = NatNet::createDataSocket( localAddress );
   
   CommandListener commandListener(sdCommand);
   commandListener.start();

   // Send a ping packet to the server so that it sends us the NatNet version
   // in its response to commandListener.
   NatNetPacket ping = NatNetPacket::pingPacket();
   ping.send(sdCommand, serverCommands);
   
   // Wait here for ping response to give us the NatNet version.
   commandListener.getNatNetVersion(natNetMajor, natNetMinor);
   std::cout << "Main thread got version " << static_cast<int>(natNetMajor) << "." << static_cast<int>(natNetMinor) << std::endl;
   
   // Start up a FrameListener, and get a reference to its output rame buffer.
   FrameListener frameListener(sdData, natNetMajor, natNetMinor);
   frameListener.start();
   boost::circular_buffer<MocapFrame>& frameBuf = frameListener.frames();
   
   // This infinite loop simulates a "worker" thread that reads the frame
   // buffer each time through.
   while(true)
   {
      while( !frameBuf.empty() )
      {
         MocapFrame frame = frameBuf.front();
         std::cout << frame << std::endl;
         frameBuf.pop_front();
      }
      
      // Sleep for a little while to simulate work :)
      usleep(100);
   }
   
   // Wait for listener threads to finish. Probably never. Just CTRL-C.
   frameListener.join();
   commandListener.join();
   
   // Epilogue
   close(sdData);
   close(sdCommand);
   return 0;
}
