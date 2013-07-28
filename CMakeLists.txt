project(ganthem CXX)
cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

# Avoid source tree pollution
If(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})
ADD_SUBDIRECTORY(src)