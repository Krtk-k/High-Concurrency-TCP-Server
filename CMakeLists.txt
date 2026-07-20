cmake_minimum_required(VERSION 3.10)
project(HighConcurrencyServer)

# Set modern C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensure the thread library is found and linked correctly
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Tell CMake where to look for header files
include_directories(include)

# Build the Server Executable
add_executable(server src/server.cpp)
target_link_libraries(server Threads::Threads)

# Build the Client Simulator Executable
add_executable(client src/clients.cpp)
target_link_libraries(client Threads::Threads)