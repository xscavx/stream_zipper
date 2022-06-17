find_package(Catch2 REQUIRED)

add_executable(tests deflate_streambuf_test.cpp)
target_compile_features(tests PRIVATE cxx_std_17)
target_link_libraries(tests PRIVATE stream_zip_library Catch2::Catch2)

add_test(NAME stream_zipper_tests COMMAND tests) # Command can be a target

