cmake_minimum_required(VERSION 2.8)
project(PSI)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14 -maes -mavx -mpclmul -mbmi2 ")

INCLUDE_DIRECTORIES( $ENV{HOME} $ENV{HOME}/libscapi/install/include
        $ENV{HOME}/libscapi/lib/OTExtensionBristol)

set(SOURCE_FILES main.cpp PartyR.cpp PartyS.cpp PartyS.h PartyR.h Defines.h)


link_directories($ENV{HOME}/libscapi/install/lib)


add_executable(PSI ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(PSI
        $ENV{HOME}/libscapi/libscapi.a gmp gmpxx
        libjsoncpp.a OTExtensionBristol libsimpleot.a
        boost_system boost_thread pthread crypto dl ssl z rt)