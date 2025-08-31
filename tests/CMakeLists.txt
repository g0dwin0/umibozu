cmake_minimum_required(VERSION 3.22)

project(umibozu_tests LANGUAGES CXX VERSION 0.0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)


file(GLOB_RECURSE SOURCES ../src/core/*.cpp ../src/mappers/*.cpp)
file(GLOB_RECURSE HEADERS ../include/*.hpp ../include/core/*.hpp)




add_executable(ppu_tests
tests.cpp
${SOURCES} ${HEADERS}
)


target_compile_options(ppu_tests PRIVATE -g -DSYSTEM_TEST_MODE)

target_include_directories(ppu_tests PRIVATE ../lib ../include ../include/core)

set_target_properties(ppu_tests PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)

set_target_properties(ppu_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_test(
    NAME ppu_tests 
    COMMAND ppu_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ppu_tests
)

target_link_libraries(ppu_tests PRIVATE Catch2::Catch2WithMain)
