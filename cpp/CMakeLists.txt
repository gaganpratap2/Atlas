add_library(atlas_core INTERFACE)

target_include_directories(
    atlas_core
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(atlas_hft
    src/main.cpp
)

target_link_libraries(
    atlas_hft
    PRIVATE
        atlas_core
)

if(ATLAS_BUILD_TESTS)
    add_executable(atlas_core_test
        tests/core_test.cpp
    )

    target_link_libraries(
        atlas_core_test
        PRIVATE
            atlas_core
    )

    add_test(
        NAME atlas_core_test
        COMMAND atlas_core_test
    )
endif()