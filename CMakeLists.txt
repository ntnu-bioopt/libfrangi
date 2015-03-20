cmake_minimum_required(VERSION 2.6)
project(libfrangi)
set(CMAKE_BUILD_TYPE Release)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
	
#compile
add_library(frangi SHARED src/frangi.cpp)
add_executable(frangi-bin example/main.cpp)
target_link_libraries(frangi-bin frangi opencv_core opencv_imgproc opencv_highgui)

#install
install (TARGETS frangi DESTINATION lib)
install (TARGETS frangi-bin DESTINATION bin)
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/frangi.h DESTINATION include)
