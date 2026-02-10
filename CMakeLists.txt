cmake_minimum_required(VERSION 3.16)
project(unleash_sdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- SDK library
file(GLOB_RECURSE UNLEASH_SDK_SOURCES
     CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(unleash_sdk STATIC ${UNLEASH_SDK_SOURCES})

target_include_directories(unleash_sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(unleash_sdk PUBLIC
  UNLEASH_SDK_VERSION="${PROJECT_VERSION}"
)

# ---- Tests (GoogleTest)
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

file(GLOB_RECURSE UNLEASH_TEST_SOURCES
     CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

add_executable(unleash_tests ${UNLEASH_TEST_SOURCES})



target_link_libraries(unleash_tests PRIVATE
    unleash_sdk
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unleash_tests)
