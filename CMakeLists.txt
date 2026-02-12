cmake_minimum_required(VERSION 3.16)
project(unleash_sdk VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: silence FetchContent timestamp warning on newer CMake
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# ---- Dependencies
include(FetchContent)

# nlohmann json
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# libcurl
find_package(CURL REQUIRED)

# GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

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

target_link_libraries(unleash_sdk PUBLIC
  nlohmann_json::nlohmann_json
  CURL::libcurl
)

enable_testing()

# ---- Tests
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
