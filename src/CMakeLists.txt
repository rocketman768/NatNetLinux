
ADD_EXECUTABLE( simple-example "SimpleExample.cpp" )
TARGET_LINK_LIBRARIES( simple-example ${Boost_LIBRARIES} )

# This one is not working yet, and sometimes for some reason does
# not end up linking with librt, which is needed by shm_unlink,
# for example.
#ADD_EXECUTABLE( shared-memory-example "SharedMemoryExample.cpp" )
#TARGET_LINK_LIBRARIES( shared-memory-example ${Boost_LIBRARIES} )
