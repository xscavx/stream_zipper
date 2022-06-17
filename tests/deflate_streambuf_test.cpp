#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <stream_zip/deflate_streambuf.hpp>

#include <iostream>
#include <string>


TEST_CASE( "Quick check", "[main]" ) {
  std::string string_in{"test string very large wowow, stp it please, please please test string very large wowow, stp it please, please please"};
  std::istringstream stringstream_in{string_in};
  zipbuf<CHUNK_SIZE> zbuf{std::cout};
  REQUIRE(true);
}
