
# ------------------------------------------------------------
# C++ Standard
# ------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# Core Library
# ------------------------------------------------------------

add_library(atlas_core STATIC

    # Market Data
    src/market_data/validator.cpp
    src/market_data/counting_processor.cpp
    src/market_data/test_feed.cpp
    src/market_data/decoder.cpp
    src/market_data/binary_recorder.cpp
    src/market_data/binary_replayer.cpp

    # Order Book
    src/order_book/order_book.cpp
    src/order_book/order_book_processor.cpp
)

target_include_directories(atlas_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(atlas_core PUBLIC cxx_std_20)


# ------------------------------------------------------------
# Main Application
# ------------------------------------------------------------

add_executable(atlas_hft
    src/main.cpp
)

target_link_libraries(atlas_hft
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Decoder Benchmark
# ------------------------------------------------------------

add_executable(atlas_decoder_benchmark
    tests/decoder_benchmark.cpp
)

target_link_libraries(atlas_decoder_benchmark
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Order Book Benchmark
# ------------------------------------------------------------

add_executable(atlas_order_book_benchmark
    tests/order_book_benchmark.cpp
)

target_link_libraries(atlas_order_book_benchmark
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Replay Benchmark
# ------------------------------------------------------------

add_executable(atlas_replay_benchmark
    tests/replay_benchmark.cpp
)

target_link_libraries(atlas_replay_benchmark
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Core Tests
# ------------------------------------------------------------

add_executable(atlas_core_test
    tests/core_test.cpp
)

target_link_libraries(atlas_core_test
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Market Data Tests
# ------------------------------------------------------------

add_executable(atlas_market_data_test
    tests/market_data_test.cpp
)

target_link_libraries(atlas_market_data_test
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Order Book Tests
# ------------------------------------------------------------

add_executable(atlas_order_book_test
    tests/order_book_test.cpp
)

target_link_libraries(atlas_order_book_test
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Order Book Metrics Benchmark
# ------------------------------------------------------------

add_executable(atlas_order_book_metrics_benchmark
    tests/order_book_metrics_benchmark.cpp
)

target_link_libraries(atlas_order_book_metrics_benchmark
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Market Data -> Order Book Integration Test
# ------------------------------------------------------------

add_executable(atlas_market_data_order_book_integration_test
    tests/market_data_order_book_integration_test.cpp
)

target_link_libraries(atlas_market_data_order_book_integration_test
    PRIVATE atlas_core
)


# ------------------------------------------------------------
# Register Tests
# ------------------------------------------------------------

enable_testing()

add_test(
    NAME atlas_core_test
    COMMAND atlas_core_test
)

add_test(
    NAME atlas_market_data_test
    COMMAND atlas_market_data_test
)

add_test(
    NAME atlas_order_book_test
    COMMAND atlas_order_book_test
)

add_test(
    NAME atlas_market_data_order_book_integration_test
    COMMAND atlas_market_data_order_book_integration_test
)


























# add_library(atlas_core INTERFACE)

# # set(CMAKE_CXX_STANDARD 20)
# # set(CMAKE_CXX_STANDARD_REQUIRED ON)
# # set(CMAKE_CXX_EXTENSIONS OFF)

# target_include_directories(
#     atlas_core
#     INTERFACE
#         ${CMAKE_CURRENT_SOURCE_DIR}/include
# )

# # Main application
# add_executable(atlas_hft
#     src/main.cpp
#     src/market_data/validator.cpp
#     src/market_data/counting_processor.cpp
#     src/market_data/test_feed.cpp
#     src/market_data/decoder.cpp
#     src/market_data/binary_recorder.cpp
#     src/market_data/binary_replayer.cpp
#     src/order_book/order_book.cpp
#     cpp/src/order_book/order_book_processor.cpp
# )

# target_link_libraries(atlas_hft PRIVATE atlas_core)

# # Decoder benchmark
# add_executable(atlas_decoder_benchmark
#     tests/decoder_benchmark.cpp
#     src/market_data/decoder.cpp
# )

# target_link_libraries(
#     atlas_decoder_benchmark
#     PRIVATE atlas_core
# )

# add_executable(atlas_order_book_benchmark
#     tests/order_book_benchmark.cpp
#     src/order_book/order_book.cpp
# )

# target_link_libraries(atlas_order_book_benchmark PRIVATE atlas_core)


# # Replay benchmark
# add_executable(atlas_replay_benchmark
#     tests/replay_benchmark.cpp
#     src/market_data/binary_recorder.cpp
#     src/market_data/binary_replayer.cpp
# )

# target_link_libraries(
#     atlas_replay_benchmark
#     PRIVATE atlas_core
# )

# # Core tests
# add_executable(atlas_core_test
#     tests/core_test.cpp
# )

# target_link_libraries(
#     atlas_core_test
#     PRIVATE atlas_core
# )

# # Market data tests
# add_executable(atlas_market_data_test
#     tests/market_data_test.cpp
#     src/market_data/validator.cpp
#     src/market_data/counting_processor.cpp
#     src/market_data/test_feed.cpp
#     src/market_data/decoder.cpp
#     src/market_data/binary_recorder.cpp
#     src/market_data/binary_replayer.cpp
# )

# target_link_libraries(
#     atlas_market_data_test
#     PRIVATE atlas_core
# )

# # Register tests
# enable_testing()

# add_test(
#     NAME atlas_core_test
#     COMMAND atlas_core_test
# )

# add_test(
#     NAME atlas_market_data_test
#     COMMAND atlas_market_data_test
# )


# add_executable(
#     atlas_order_book_test
#     tests/order_book_test.cpp
#     src/order_book/order_book.cpp
# )

# target_link_libraries(
#     atlas_order_book_test
#     PRIVATE
#     atlas_core
# )

# add_test(
#     NAME atlas_order_book_test
#     COMMAND atlas_order_book_test
# )


# add_executable(atlas_order_book_metrics_benchmark
#     tests/order_book_metrics_benchmark.cpp
#     src/order_book/order_book.cpp
# )

# target_link_libraries(atlas_order_book_metrics_benchmark PRIVATE atlas_core)


# add_executable(
#     atlas_market_data_order_book_integration_test
#     tests/market_data_order_book_integration_test.cpp
# )

# target_link_libraries(
#     atlas_market_data_order_book_integration_test
#     PRIVATE
#     atlas_hft
# )

# add_test(
#     NAME atlas_market_data_order_book_integration_test
#     COMMAND atlas_market_data_order_book_integration_test
# )