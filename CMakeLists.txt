cmake_minimum_required(VERSION 3.16)
project(XVisioStreamer)

set(CMAKE_CXX_STANDARD 14)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug)
ENDIF()

find_package(OpenCV  REQUIRED)
find_package(Boost COMPONENTS filesystem serialization REQUIRED)
find_package(Threads REQUIRED)
find_package(xvsdk REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 ")


add_library(xvisio
   include/SLAMFrame.h
   src/tick.cc
   src/XVisio.cc
   src/AtomicTimestampOffset.cc
   src/XVisioStreamServer.cc
   src/XVisioStreamClient.cc
)

target_include_directories(xvisio
   PUBLIC include
   PUBLIC ${OpenCV_INCLUDE_DIRS}
   PUBLIC ${xvsdk_INCLUDE_DIRS}
)

target_link_libraries(xvisio
   PUBLIC ${xvsdk_LIBRARIES}
   PUBLIC ${OpenCV_LIBS}
   PRIVATE Boost::boost
   PRIVATE Boost::filesystem
   PRIVATE Boost::serialization
   PRIVATE Threads::Threads # Check if it has access to this and resolve thread lockup?
)

add_executable(XVisioStreamer
	src/XVisioStreamer.cc
)

target_link_libraries(XVisioStreamer
   PRIVATE xvisio
   PRIVATE Boost::boost
   PRIVATE Boost::filesystem
   PRIVATE Boost::serialization
   PRIVATE Threads::Threads
)

add_executable(XVisioStreamReceiver
	src/XVisioStreamReceiver.cc
)

target_link_libraries(XVisioStreamReceiver
   PRIVATE xvisio
   PRIVATE Boost::boost
   PRIVATE Boost::filesystem
   PRIVATE Boost::serialization
   PRIVATE Threads::Threads
)
