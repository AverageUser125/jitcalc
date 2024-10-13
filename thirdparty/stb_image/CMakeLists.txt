cmake_minimum_required(VERSION 3.10)
project(stb_image)

# Set the CMP0069 policy, because warning annoying
if(POLICY CMP0069)
    cmake_policy(SET CMP0069 NEW)
endif()

add_library(stb_image)
target_sources(stb_image PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/stb_image.cpp")
target_include_directories(stb_image PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")



