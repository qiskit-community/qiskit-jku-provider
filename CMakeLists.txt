cmake_minimum_required (VERSION 3.0)
project( quantum_simulator )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

SET(CMAKE_CXX_FLAGS "-Wall -g3 -O3 -D_REENTRANT -DVERSION=\"$(VERSION)\" -std=c++11")

FIND_PACKAGE( Boost 1.50 COMPONENTS program_options REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

FIND_PACKAGE(MPFR REQUIRED)
FIND_PACKAGE(MPFR++ REQUIRED)
INCLUDE_DIRECTORIES(${MPFR_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${MPFR_LIBRARIES})

add_executable(quantum_simulator src/qmdd_simulator.cpp src/QASM_scanner.cpp src/QMDDpackage.cpp src/textFileUtilities.cpp src/qcost.cpp src/timing.cpp  src/QMDDcircuit.cpp src/QMDDcomplexD.cpp src/QMDDreorder.cpp)
include_directories(src)
target_link_libraries(quantum_simulator mpfr boost_program_options)
