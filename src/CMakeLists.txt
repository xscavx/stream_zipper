add_executable(test test.cpp)
find_package(ZLIB REQUIRED)
target_link_libraries(test PRIVATE ZLIB::ZLIB)

target_include_directories(test PUBLIC ../include)

target_compile_features(test PUBLIC cxx_std_17)
set_target_properties(test PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(test PRIVATE -Weverything -Wno-c++98-compat)
