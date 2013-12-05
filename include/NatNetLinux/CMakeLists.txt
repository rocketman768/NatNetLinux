SET( H_FILES
   "NatNetPacket.h"
   "NatNetSender.h"
)

INSTALL(
   FILES ${H_FILES}
   DESTINATION include/NatNetLinux
)
