cmake_minimum_required(VERSION 2.8)
PROJECT(Note)

# define release or debug
IF (CMAKE_BUILD_TYPE)
ELSE ()
    SET(CMAKE_BUILD_TYPE "Debug")
ENDIF ()

# define version
if (DEFINED MS_VERSION)
    message(STATUS "ISE version is ${MS_VERSION}")
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MS_VERSION "debug")
endif ()

# invoke conan install command
#SET(CONAN_FILE_NAME "conanfile.txt")
#execute_process(COMMAND conan install ${PROJECT_SOURCE_DIR}/${CONAN_FILE_NAME} --build=missing)
#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
#conan_basic_setup()

SET(CMAKE_CXX_FLAGS_DEBUG " $ENV{CXXFLAGS} -std=c++11 -DDEBUG -DMS_VERSION=${MS_VERSION} -g -O0 -w")
SET(CMAKE_CXX_FLAGS_RELEASE " $ENV{CXXFLAGS} -std=c++11 -DNDEBUG -DMS_VERSION=${MS_VERSION} -O3 -w")

# exe proto file
#execute_process(COMMAND ./proto_gen.sh cpp WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/imodel/proto/)

# add include path
INCLUDE_DIRECTORIES(${CONAN_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/module/boost/include
        ${PROJECT_SOURCE_DIR}/module/json/include
        ${PROJECT_SOURCE_DIR}/module/glog/include
        ${PROJECT_SOURCE_DIR}/module/minclude
        ${PROJECT_SOURCE_DIR}/module/pugixml
        ${PROJECT_SOURCE_DIR}/module/watch)

# add lib path
SET(PLATFORM ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})

foreach (libdir ${CONAN_LIB_DIRS})
    LIST(APPEND LIBSDIR ${libdir}/${PLATFORM})
endforeach (libdir)
LINK_DIRECTORIES(${LIBSDIR} ${CONAN_LIB_DIRS})

LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/module/boost/lib
        ${PROJECT_SOURCE_DIR}/module/json/lib
        ${PROJECT_SOURCE_DIR}/module/glog/lib)

# add lib
#FIND_PACKAGE(Boost COMPONENTS regex system filesystem coroutine context thread REQUIRED)
LIST(APPEND LIBS
        ${CONAN_LIBS_PROTOBUF}
        ${CONAN_LIBS_GRPC}
        ${CONAN_LIBS_GRPCPP})
LIST(APPEND LIBS ${Boost_LIBRARIES} glog gflags pthread json)

# add module
ADD_SUBDIRECTORY(json)
ADD_SUBDIRECTORY(pugixml)
ADD_SUBDIRECTORY(flow)
ADD_SUBDIRECTORY(paradigm)