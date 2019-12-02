cmake_minimum_required (VERSION 3.0)
project(jku_simulator LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Set default build type to Release
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: Debug Release"
        FORCE)
ENDIF()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_REENTRANT -DVERSION=\"$(VERSION)\"")

IF(STATIC_LINKING)
    IF(NOT APPLE)
        SET(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
    ENDIF()
    SET(CMAKE_FIND_LIBRARY_SUFFIXES .a)
    IF(WIN32)
        LIST(INSERT CMAKE_FIND_LIBRARY_SUFFIXES 0 .lib .a)
    ENDIF()
ENDIF()

IF(APPLE)
    SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "" FORCE)
ENDIF()

if(STATIC_LINKING)
    SET(Boost_USE_STATIC_LIBS ON)
ENDIF()
FIND_PACKAGE(Boost 1.50 COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

FIND_PACKAGE(MPFR REQUIRED)
FIND_PACKAGE(MPFR++ REQUIRED)
INCLUDE_DIRECTORIES(${MPFR_INCLUDE_DIRS})

SET(JKU_LIBS
    ${MPFR_LIBRARIES}
    ${Boost_LIBRARIES})

add_executable(jku_simulator
    src/main.cpp
    src/Simulator.cpp
    src/QASMsimulator.cpp
    src/QASMscanner.cpp
    src/QASMtoken.cpp
    src/QMDDpackage.cpp
    src/textFileUtilities.cpp
    src/qcost.cpp
    src/timing.cpp
    src/QMDDcircuit.cpp
    src/QMDDcomplexD.cpp
    src/QMDDreorder.cpp)

set_target_properties(jku_simulator PROPERTIES
    LINKER_LANGUAGE CXX
    CXX_STANDARD 14)

IF(STATIC_LINKING)
    set_target_properties(jku_simulator PROPERTIES
        LINK_SEARCH_END_STATIC True)
ENDIF()


target_link_libraries(jku_simulator ${JKU_LIBS})

include_directories(src)

file(COPY        ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
