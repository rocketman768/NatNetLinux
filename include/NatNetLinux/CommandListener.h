/*
 * CommandListener.h is part of NatNetLinux, and is Copyright 2013,
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

#ifndef COMMANDLISTENER_H
#define COMMANDLISTENER_H

#include <NatNetLinux/NatNet.h>
#include <NatNetLinux/NatNetPacket.h>
#include <NatNetLinux/NatNetSender.h>
#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>

/*!
 * \brief Class to listen for command responses.
 * 
 * This class spawns a new thread to listen for command responses.
 */
class CommandListener
{
public:
   
   /*!
    * \brief Constructor
    * 
    * \param sd The socket on which to listen.
    */
   CommandListener(int sd = -1) :
      _thread(0),
      _sd(sd),
      _nnMajor(0),
      _nnMinor(0)
   {
      _nnVersionMutex.lock();
   }
   
   ~CommandListener()
   {
      delete _thread;
   }
   
   //! \brief Begin the listening in new thread. Non-blocking.
   void start()
   {
      _thread = new boost::thread( &CommandListener::_work, this, _sd);
   }
   
   //! \brief Wait for the listening thread to stop. Blocking.
   void join()
   {
      if(_thread)
         _thread->join();
   }
   
   /*!
    * \brief Get NatNet major and minor version numbers. Blocking.
    * 
    * WARNING: this call blocks until a ping response packet is heard.
    * The reason is that the ping response packet is the only one that
    * contains the NatNet version string. So, you \b MUST send a ping to
    * the server before calling this to avoid deadlock.
    * 
    * \param major output NatNet major version
    * \param minor output NatNet minor version
    */
   void getNatNetVersion( unsigned char& major, unsigned char& minor )
   {
      _nnVersionMutex.lock();
      major = _nnMajor;
      minor = _nnMinor;
      _nnVersionMutex.unlock();
   }
   
private:
   
   boost::thread* _thread;
   int _sd;
   unsigned char _nnMajor;
   unsigned char _nnMinor;
   boost::mutex _nnVersionMutex;
   
   void _work(int sd)
   {
      char const* response;
      ssize_t len;
      NatNetPacket nnp;
      struct sockaddr_in senderAddress;
      socklen_t senderAddressLength = sizeof(senderAddress);
      NatNetSender sender;
      
      while(true)
      {
         // Give other threads an opportunity to interrupt this thread.
         //boost::this_thread::interruption_point();
       
         // blocking
         len = recvfrom(
            sd,
            nnp.rawPtr(), nnp.maxLength(),
            0, reinterpret_cast<struct sockaddr*>(&senderAddress), &senderAddressLength
         );

         if(len <= 0)
            continue;

         switch(nnp.iMessage())
         {
         case NatNetPacket::NAT_MODELDEF:
            //Unpack(nnp.rawPtr());
            break;
         case NatNetPacket::NAT_FRAMEOFDATA:
            //Unpack(nnp.rawPtr());
            break;
         case NatNetPacket::NAT_PINGRESPONSE:
            sender.unpack(nnp.read<char>(0));
            _nnMajor = sender.natNetVersion()[0];
            _nnMinor = sender.natNetVersion()[1];
            _nnVersionMutex.unlock();
            std::cout << "[Client] Server Software: " << sender.name() << std::endl;
            printf("[Client] NatNetVersion: %d.%d\n",sender.natNetVersion()[0],sender.natNetVersion()[1]);
            printf("[Client] ServerVersion: %d.%d\n",sender.version()[0],sender.version()[1]);
            break;
         case NatNetPacket::NAT_RESPONSE:
            response = nnp.read<char>(0);
            printf("Response : %s", response);
            break;
         case NatNetPacket::NAT_UNRECOGNIZED_REQUEST:
            printf("[Client] received 'unrecognized request'\n");
            break;
         case NatNetPacket::NAT_MESSAGESTRING:
            response = nnp.read<char>(0);
            printf("[Client] Received message: %s\n", response);
            break;
         default:
            break;
        } // end switch(nnp.iMessage)
        
      }
   }
};

#endif /*COMMANDLISTENER_H*/
