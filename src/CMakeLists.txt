add_executable(
  stream_zipper
  stream_zipper.cxx
  deflate_streambuf.cxx
)

find_package(ZLIB)
target_link_libraries(stream_zipper ZLIB::ZLIB)

target_compile_features(stream_zipper PUBLIC cxx_std_17)
set_target_properties(stream_zipper PROPERTIES CXX_EXTENSIONS OFF)
