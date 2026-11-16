
add_library(atlas_core INTERFACE)

target_include_directories(
    atlas_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Main application
add_executable(atlas_hft
    src/main.cpp
    src/market_data/validator.cpp
    src/market_data/counting_processor.cpp
    src/market_data/test_feed.cpp
    src/market_data/decoder.cpp
)

target_link_libraries(
    atlas_hft
    PRIVATE
        atlas_core
)

add_executable(atlas_decoder_benchmark
    tests/decoder_benchmark.cpp
    src/market_data/decoder.cpp
)
target_include_directories(atlas_decoder_benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Core tests
add_executable(atlas_core_test
    tests/core_test.cpp
)

target_link_libraries(
    atlas_core_test
    PRIVATE
        atlas_core
)

# Market data tests
add_executable(atlas_market_data_test
    tests/market_data_test.cpp
    src/market_data/validator.cpp
    src/market_data/counting_processor.cpp
    src/market_data/test_feed.cpp
    src/market_data/decoder.cpp
)

target_link_libraries(
    atlas_market_data_test
    PRIVATE
        atlas_core
)

# Register tests
add_test(
    NAME atlas_core_test
    COMMAND atlas_core_test
)

add_test(
    NAME atlas_market_data_test
    COMMAND atlas_market_data_test
)
