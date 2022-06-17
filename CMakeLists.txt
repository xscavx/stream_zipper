cmake_minimum_required(VERSION 3.8)

project(
  stream_zip
  VERSION 1.0
  LANGUAGES CXX
)

add_subdirectory(src)

include(CTest)
add_subdirectory(tests)
