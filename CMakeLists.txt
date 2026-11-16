cmake_minimum_required(VERSION 3.20)

project(
    ATLAS_HFT
    VERSION 0.1.0
    DESCRIPTION "AI-Assisted Trading, Low-Latency Analytics & Systematic Research Platform"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ATLAS_BUILD_TESTS "Build ATLAS-HFT tests" ON)

add_subdirectory(cpp)

if(ATLAS_BUILD_TESTS)
    enable_testing()
endif()

