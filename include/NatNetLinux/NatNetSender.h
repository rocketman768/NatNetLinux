#ifndef NATNETSENDER_H
#define NATNETSENDER_H

#include <string.h>
#include <string>

/*!
 * \brief Class to encapsulate NatNet Sender packet data.
 */
class NatNetSender
{
#define MAX_NAMELENGTH 256
public:
   NatNetSender()
   {
      memset(_name, 0, MAX_NAMELENGTH);
      memset(_version, 0, 4);
      memset(_natNetVersion, 0, 4);
   }
   
   ~NatNetSender(){}
   
   //! \brief Name of sending application.
   std::string name() const
   {
      return _name;
   }
   
   //! \brief Length 4 array version number of sending application (major.minor.build.revision)
   unsigned char const* version() const
   {
      return _version;
   }
   
   //! \brief Length 4 array version number of sending application's NatNet version (major.minor.build.revision)
   unsigned char const* natNetVersion() const
   {
      return _natNetVersion;
   }
   
   //! \brief Unpack the class from raw pointer.
   void unpack(char const* data)
   {
      // NOTE: do we have to worry about network order data? I.e. ntohs() and stuff?
      strncpy( _name, data, MAX_NAMELENGTH );
      data += MAX_NAMELENGTH;
      _version[0]       = data[0];
      _version[1]       = data[1];
      _version[2]       = data[2];
      _version[3]       = data[3];
      _natNetVersion[0] = data[4];
      _natNetVersion[1] = data[5];
      _natNetVersion[2] = data[6];
      _natNetVersion[3] = data[7];
   }
   
private:
   
   char _name[MAX_NAMELENGTH];
   unsigned char _version[4];
   unsigned char _natNetVersion[4];
   
#undef MAX_NAMELENGTH
};

#endif /*NATNETSENDER_H*/
