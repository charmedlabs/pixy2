cmake_minimum_required (VERSION 2.8)
project (get_vectors CXX)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake" )

# Add sources here... #
add_executable (get_vectors get_vectors_c_demo.cpp)

# libpixyusb should always come before libboost and libusb #
target_link_libraries (get_vectors pixyusb)

find_package ( libpixyusb REQUIRED )
find_package ( libusb-1.0 REQUIRED )
find_package ( Boost 1.49 COMPONENTS system chrono REQUIRED)

target_link_libraries (get_vectors ${Boost_LIBRARIES})
target_link_libraries (get_vectors ${LIBUSB_1_LIBRARY})

file(STRINGS "cmake/VERSION" LIBPIXY_VERSION)
add_definitions(-D__LIBPIXY_VERSION__="${LIBPIXY_VERSION}")

include_directories (src
                     include
                     ../../common
                     ${Boost_INCLUDE_DIR}
                     ${LIBUSB_1_INCLUDE_DIRS})

