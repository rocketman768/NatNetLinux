/*
 * SharedMemoryExample.cpp is part of NatNetLinux, and is Copyright 2014,
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

#include <boost/program_options.hpp>

void readOpts(
   uint32_t& localAddress,
   uint32_t& serverAddress,
   std::string& shmName,
   int argc, char* argv[]
)
{
   namespace po = boost::program_options;
   
   po::options_description desc("shared-memory-example: reads NatNetLinux packets into a shared memory buffer\nOptions");
   desc.add_options()
      ("help", "Display help message")
      ("local-addr,l", po::value<std::string>(), "Local IPv4 address")
      ("server-addr,s", po::value<std::string>(), "Server IPv4 address")
      ("shared-memory-name,m", po::value<std::string>(), "Shared memory name for frame buffer")
   ;
   
   po::variables_map vm;
   po::store(po::parse_command_line(argc,argv,desc), vm);
   
   if(
      argc < 7 || vm.count("help") ||
      !vm.count("local-addr") ||
      !vm.count("server-addr") ||
      !vm.count("shared-memory-name")
   )
   {
      std::cout << desc << std::endl;
      exit(1);
   }
   
   localAddress = inet_addr( vm["local-addr"].as<std::string>().c_str() );
   serverAddress = inet_addr( vm["server-addr"].as<std::string>().c_str() );
   shmName = vm["shared-memory-name"].as<std::string>();
}

int main(int argc, char* argv[])
{
   unsigned char natNetMajor;
   unsigned char natNetMinor;
   
   int sdCommand;
   int sdData;
   
   uint32_t localAddress;
   uint32_t serverAddress;
   std::string shmName;
   
   readOpts( localAddress, serverAddress, shmName, argc, argv );
   
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
   
   // Start up a FrameListener, and get a reference to its output frame buffer.
   FrameListener frameListener(sdData, natNetMajor, natNetMinor, 64, shmName.c_str() );
   frameListener.start();
   
   // The FrameListener will just run forever putting data into the shared
   // memory buffer. A "clent" process would do the following:
   /*
   using namespace boost::interprocess;
   shared_memory_object shm(open_only, shmName.c_str(), read_write);
   mapped_region region(shm, read_write);
   boost::circular_buffer<MocapFrame>* frames =
      static_cast<  boost::circular_buffer<MocapFrame>* >(region.get_address());
   */
   
   // Wait for listener threads to finish. Probably never. Just CTRL-C.
   frameListener.join();
   commandListener.join();
   
   // Epilogue
   close(sdData);
   close(sdCommand);
   return 0;
}
