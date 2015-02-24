cmake_minimum_required(VERSION 2.6)
project(libfrangi)
set(CMAKE_BUILD_TYPE Release)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
	
#compile
add_library(frangi SHARED frangi.cpp)

#install
install (TARGETS frangi DESTINATION lib)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/frangi.h DESTINATION include)
