cmake_minimum_required(VERSION 3.15)

project(SmartStrike
    VERSION 0.1.0
    LANGUAGES CXX
)

# -------------------------------------------------------
# Global settings
# -------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing (tests/CMakeLists.txt will hook into this)
enable_testing()

# -------------------------------------------------------
# Dependencies
# -------------------------------------------------------

# OpenCV
find_package(OpenCV REQUIRED core imgproc highgui videoio)

# yaml-cpp
find_package(yaml-cpp REQUIRED)

# GTest is only needed for tests; we'll handle it in tests/CMakeLists.txt
# but we can optionally find it here as well.
find_package(GTest QUIET)

# -------------------------------------------------------
# Include directories
# -------------------------------------------------------
# Our public headers live in include/strikezone/*.hpp
set(SMARTSTRIKE_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include_directories(${SMARTSTRIKE_INCLUDE_DIR})

# -------------------------------------------------------
# Source files
# -------------------------------------------------------

set(SMARTSTRIKE_SOURCES
    src/ball_detector.cpp
    src/ball_tracker.cpp
    src/batter_detector.cpp
    src/camera_capture.cpp
    src/config.cpp
    src/decision_engine.cpp
    src/frame_processor.cpp
    src/logger.cpp
    src/pipeline.cpp
    src/plate_detector.cpp
    src/strike_zone_model.cpp
    src/visualizer.cpp
)

# -------------------------------------------------------
# Core library
# -------------------------------------------------------

add_library(smartstrike STATIC
    ${SMARTSTRIKE_SOURCES}
)

target_include_directories(smartstrike
    PUBLIC
        ${SMARTSTRIKE_INCLUDE_DIR}
)

# Link against OpenCV + yaml-cpp
target_link_libraries(smartstrike
    PUBLIC
        ${OpenCV_LIBS}
        yaml-cpp
)

# -------------------------------------------------------
# Executables
# -------------------------------------------------------

# Realtime app
add_executable(strikezone_realtime
    app/main_realtime.cpp
)

target_link_libraries(strikezone_realtime
    PRIVATE
        smartstrike
)

# Offline (video) app
add_executable(strikezone_offline
    app/main_offline.cpp
)

target_link_libraries(strikezone_offline
    PRIVATE
        smartstrike
)

# -------------------------------------------------------
# Tests
# -------------------------------------------------------

# Only add tests subdirectory if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
