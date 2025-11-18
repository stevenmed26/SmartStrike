cmake_minimum_required(VERSION 3.15)

# This CMakeLists expects:
# - The core library "smartstrike" to be defined in the parent directory.
# - enable_testing() to already be called in the root CMakeLists.

project(SmartStrikeTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------
# Dependencies
# -------------------------------------------------------

# GoogleTest
find_package(GTest REQUIRED)

# OpenCV (for synthetic image creation in tests)
find_package(OpenCV REQUIRED core imgproc highgui)

# Bring in GoogleTest helpers (gtest_discover_tests)
include(GoogleTest)

# -------------------------------------------------------
# Test sources
# -------------------------------------------------------

set(SMARTSTRIKE_TEST_SOURCES
    test_ball_detector.cpp
    test_decision_engine.cpp
    test_plate_detector.cpp
)

add_executable(smartstrike_tests
    ${SMARTSTRIKE_TEST_SOURCES}
)

target_include_directories(smartstrike_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(smartstrike_tests
    PRIVATE
        smartstrike
        GTest::gtest
        GTest::gtest_main
        ${OpenCV_LIBS}
)

# -------------------------------------------------------
# Register tests with CTest
# -------------------------------------------------------

gtest_discover_tests(smartstrike_tests)
