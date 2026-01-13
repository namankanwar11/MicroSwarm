cmake_minimum_required(VERSION 3.14)
project(MicroSwarm VERSION 0.1.0 LANGUAGES CXX C)

# --- USE C++17 (STABLE) ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Hardware Acceleration ---
set(LLAMA_CUDA OFF)
set(GGML_CUDA OFF) 
set(LLAMA_METAL OFF)

# --- llama.cpp Options ---
set(LLAMA_BUILD_COMMON ON)
set(LLAMA_BUILD_TESTS OFF)
set(LLAMA_BUILD_EXAMPLES OFF)
set(LLAMA_BUILD_SERVER OFF)
set(LLAMA_CURL OFF) 

# Add Submodule
add_subdirectory(external/llama.cpp)

# --- MicroSwarm Executable ---
# Add the source files here
# Include directories for httplib
include_directories(external/llama.cpp/vendor/cpp-httplib)

add_executable(microswarm 
    src/main.cpp 
    src/LLMEngine.cpp 
    src/LLMEngine.h
    src/SwarmOrchestrator.cpp
    src/SwarmOrchestrator.h
    src/Agent.h
    src/APIServer.h
)

target_include_directories(microswarm PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/common
)

target_link_libraries(microswarm PRIVATE llama common)