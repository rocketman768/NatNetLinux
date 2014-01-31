/*
 * FrameListener.h is part of NatNetLinux, and is Copyright 2013,
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

#ifndef FRAMELISTENER_H
#define FRAMELISTENER_H

#include <NatNetLinux/NatNet.h>
#include <NatNetLinux/NatNetPacket.h>
#include <NatNetLinux/NatNetSender.h>
#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>

/*!
 * \brief Class to listen for MocapFrame data.
 * 
 * This class uses a circular buffer to store the frame data. There is no need
 * for synchronization across threads to this buffer as long as the frames
 * are copied or used quickly enough so that this class does not wrap back
 * around and overwrite them. Otherwise, you may consider adding a mutex and
 * lock for accessing the buffer.
 * 
 */
class FrameListener
{
public:
   
   /*!
    * \brief Constructor
    * 
    * \param sd The socket on which to listen.
    * \param nnMajor NatNet major version.
    * \param nnMinor NatNet minor version.
    * \param bufferSize number of frames in the \c frames() buffer.
    */
   FrameListener(int sd = -1, unsigned char nnMajor=0, unsigned char nnMinor=0, size_t bufferSize=64 ) :
      _thread(0),
      _sd(sd),
      _nnMajor(nnMajor),
      _nnMinor(nnMinor),
      _frames(bufferSize),
      _run(false)
   {
   }
   
   ~FrameListener()
   {
      if( running() )
         stop();
      // I think this may be blocking unless the thread is stopped.
      delete _thread;
   }
   
   //! \brief Begin the listening in new thread. Non-blocking.
   void start()
   {
      _run = true;
      _thread = new boost::thread( &FrameListener::_work, this, _sd);
   }
   
   //! \brief Cause the thread to stop. Non-blocking.
   void stop()
   {
      _run = false;
   }
   
   //! \brief Return true iff the listener thread is running. Non-blocking.
   bool running()
   {
      return _run;
   }
   
   //! \brief Wait for the listening thread to stop. Blocking.
   void join()
   {
      if(_thread)
         _thread->join();
   }
   
   //! \brief Circular buffer that contains the frames.
   boost::circular_buffer<MocapFrame>& frames() { return _frames; }
   
private:
   
   boost::thread* _thread;
   int _sd;
   unsigned char _nnMajor;
   unsigned char _nnMinor;
   boost::circular_buffer<MocapFrame> _frames;
   bool _run;
   
   void _work(int sd)
   {
      NatNetPacket nnp;
      while(_run)
      {
         int dataBytes = read( sd, nnp.rawPtr(), nnp.maxLength() );
         
         std::cout << "Got " << dataBytes << " B\n";
         if( dataBytes > 0 && nnp.iMessage() == NatNetPacket::NAT_FRAMEOFDATA )
         {
            MocapFrame mFrame(_nnMajor,_nnMinor);
            mFrame.unpack(nnp.rawPayloadPtr());
            _frames.push_back(mFrame);
         }
      }
   }
};

#endif /*FRAMELISTENER_H*/
