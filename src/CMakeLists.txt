cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(ganthem CommandLineOptions.cpp ganthem.cpp ANT.cpp ANTPlus.cpp FIT.cpp GarminConvert.cpp GPX.cpp Log.cpp SerialIO.cpp)
target_link_libraries (ganthem pthread) 
