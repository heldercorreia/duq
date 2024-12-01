# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name and language
project(duq C)

# Set the C standard to C17
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set the default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags for Release build
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -flto -march=native -mtune=native -funroll-loops -Wall -pedantic")

# Define the version as a preprocessor macro
set(DUQ_VERSION "1.0")
ADD_DEFINITIONS("-DDUQ_VERSION=\"${DUQ_VERSION}\"")

# Source files
set(SRCS duq/duq.c)

# Add the executable target
add_executable(duq ${SRCS})

# Linker flags to strip the binary
set_target_properties(duq PROPERTIES LINK_FLAGS "-s")

# Standard install step to install the executable
install(TARGETS duq
    RUNTIME DESTINATION bin
)
