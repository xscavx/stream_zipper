#define CATCH_CONFIG_MAIN

#include <iostream>
#include <string>
#include <catch2/catch.hpp>
#include <stream_zip/deflate_streambuf.hpp>


TEST_CASE( "Zipper stream testing", "[main]" ) {
  static constexpr std::string_view string_in{"test string: zip me please"};
  static constexpr auto iterations = 30000;
  static constexpr auto in_buf_size = string_in.size() * iterations;

  std::cout << "source string:\n" << string_in << std::endl;
  std::cout << "final string:\n";

  {
    zipbuf<300000, 600000> zbuf{std::cout, Z_NO_COMPRESSION};
    std::ostream zip_output(&zbuf);
    for (int i = 0; i < iterations; ++i) {
      zip_output << string_in << " ";
    }
    zbuf.zflush();
  }

  std::cout << std::endl;
  REQUIRE(false);
}
