# EnableTests.cmake - Configure GoogleTest for Cloudbus/segment
#
# This module configures GoogleTest when CB_SEGMENT_ENABLE_TESTS is ON.
# It fetches GoogleTest, sets up testing, and adds the tests subdirectory.
message(STATUS "Configure tests with GoogleTest")

# Fetch GoogleTest using FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Make GoogleTest available
FetchContent_MakeAvailable(googletest)

# Add the tests subdirectory
add_subdirectory(tests)

message(STATUS "GoogleTest configured successfully")
