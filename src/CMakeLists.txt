add_library(
  stream_zip_library
  ../include/stream_zip/deflate_streambuf.hpp
  deflate_streambuf.cpp
)

find_package(ZLIB REQUIRED)
target_link_libraries(stream_zip_library PRIVATE ZLIB::ZLIB)
target_include_directories(stream_zip_library PUBLIC ../include)
set_target_properties(stream_zip_library PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(stream_zip_library PUBLIC cxx_std_17)
target_compile_options(
  stream_zip_library
  PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Werror

  -Weverything
  -Wno-c++98-compat
)
