add_executable(tests deflate_streambuf_test.cpp)

find_package(Catch2 REQUIRED)
target_link_libraries(tests PRIVATE stream_zip_library Catch2::Catch2)

target_compile_features(tests PRIVATE cxx_std_17)
set_target_properties(stream_zip_library PROPERTIES CXX_EXTENSIONS OFF)

add_test(NAME stream_zipper_tests COMMAND tests) # Command can be a target

