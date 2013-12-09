#ifndef NATNETPACKET_H
#define NATNETPACKET_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

/*!
 * \brief Class to encapsulate NatNet packets.
 */
class NatNetPacket
{
public:
   
#define MAX_PACKETSIZE 100000
   
   //! \brief Message types
   enum NatNetMessageID
   {
      NAT_PING                 = 0,
      NAT_PINGRESPONSE         = 1,
      NAT_REQUEST              = 2,
      NAT_RESPONSE             = 3,
      NAT_REQUEST_MODELDEF     = 4,
      NAT_MODELDEF             = 5,
      NAT_REQUEST_FRAMEOFDATA  = 6,
      NAT_FRAMEOFDATA          = 7,
      NAT_MESSAGESTRING        = 8,
      NAT_UNRECOGNIZED_REQUEST = 100
   };
   
   //! \brief Default constructor.
   NatNetPacket()
      : _data(new char[MAX_PACKETSIZE+4]), _dataLen(MAX_PACKETSIZE+4)
   {
   }
   
   ~NatNetPacket()
   {
      delete[] _data;
   }
   
   //! \brief Construct a "ping" packet.
   static NatNetPacket pingPacket()
   {
      NatNetPacket packet;
      
      uint16_t m = NAT_PING;
      uint16_t len = 0;
      
      *reinterpret_cast<uint16_t*>(packet._data)   = m;
      *reinterpret_cast<uint16_t*>(packet._data+2) = len;
      
      return packet;
   }
   
   /*!
    * \brief Send packet over the series of tubes.
    * \param sd Socket to use (already bound to an address)
    */
   int send(int sd) const
   {
      // Have to prepend '::' to avoid conflicting with NatNetPacket::send().
      return ::send(sd, _data, 4+nDataBytes(), 0);
   }
   
   /*! \brief Send packet over the series of tubes.
    * \param sd Socket to use
    * \param destAddr Address to which to send the packet
    */
   int send(int sd, struct sockaddr_in destAddr) const
   {
      return sendto(sd, _data, 4+nDataBytes(), 0, (sockaddr*)&destAddr, sizeof(destAddr));
   }
   
   //! \brief Return a raw pointer to the packet data. Careful.
   char* rawPtr()
   {
      return _data;
   }
   
   const char* rawPtr() const
   {
      return _data;
   }
   
   const char* rawPayloadPtr() const
   {
      return _data+4;
   }
   
   /*!
    * \brief Maximum length of the underlying packet data.
    * 
    * \c rawPtr()[\c maxLength()-1] should be a good dereference.
    */
   size_t maxLength() const
   {
      return _dataLen;
   }
   
   //! \brief Get the message type.
   NatNetMessageID iMessage() const
   {
      unsigned short m = *reinterpret_cast<unsigned short*>(_data);
      return static_cast<NatNetMessageID>(m);
   }
   
   //! \brief Get the number of bytes in the payload.
   unsigned short nDataBytes() const
   {
      return *reinterpret_cast<unsigned short*>(_data+2);
   }
   
   /*!
    * \brief Read payload data. Const version.
    * \param offset payload byte offset
    */
   template<class T> T const* read( size_t offset ) const
   {
      // NOTE: need to worry about network byte order?
      return reinterpret_cast<T*>(_data+4+offset);
   }
   
   /*!
    * \brief Read payload data.
    * \param offset payload byte offset
    */
   template<class T> T* read( size_t offset )
   {
      // NOTE: need to worry about network byte order?
      return reinterpret_cast<T*>(_data+4+offset);
   }
   
private:
   
   char* _data;
   size_t _dataLen;

#undef MAX_PACKETSIZE
};

#endif /*NATNETPACKET_H*/
