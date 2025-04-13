cmake_minimum_required(VERSION 3.10)

# Project name and version
project(strutil VERSION 1.0 LANGUAGES C)

# Set the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Add the library
add_library(strutil STATIC src/strutil.c)

# Add subdirectories
add_subdirectory(example)
add_subdirectory(tests)