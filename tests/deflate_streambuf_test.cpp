#define CATCH_CONFIG_MAIN

#include <iostream>
#include <sstream>
#include <string>
#include <catch2/catch.hpp>
#include <stream_zip/deflate_streambuf.hpp>


TEST_CASE( "Deflate streambuf simple test", "[main]" ) {
  static constexpr std::string_view string_in{"test string: zip me please"};
  static constexpr std::string_view string_in2{"second part of the party"};

  static constexpr auto input_iterations = 100;
  static constexpr auto split_to_buffers = 3;
  static constexpr auto buf_size = (string_in.size() * input_iterations
                                    / split_to_buffers) + 1;

  std::cout << "source string:\n" << string_in << std::endl;
  std::cout << "final string:\n";
  std::ostringstream output{};

  {
    zstream::zipstreambuf<buf_size> zsbuf{output, Z_NO_COMPRESSION};
    std::ostream zip_output(&zsbuf);
    for (int i = 0; i < input_iterations; ++i) {
      zip_output << string_in << " ";
    }
    zsbuf.zflush();
  }

  auto const str = output.str();
  std::cout << "[" << str << "]" << std::endl;
  REQUIRE(false);
}
