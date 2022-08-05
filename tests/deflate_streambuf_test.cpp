#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#include <zlib.h>

#include <stream_zip/deflate_streambuf.hpp>

inline constexpr size_t BUF_SIZE{16536};
inline constexpr std::string_view string_to_deflate{"test string: zip me please"};

TEST_CASE("Deflate streambuf no compression size", "[main]") {
  static constexpr size_t iterations = 5;

  std::ostringstream output;
  zstream::zipstreambuf<BUF_SIZE> zsbuf{output, Z_NO_COMPRESSION};
  std::ostream zip_output{&zsbuf};

  for (int i = 0; i < iterations; ++i) {
    zip_output << string_to_deflate;
  }
  zsbuf.zflush();

  REQUIRE(output.str().size() >= string_to_deflate.size() * iterations);
}

TEST_CASE("Deflate streambuf best compression size", "[main]") {
  static constexpr size_t iterations = 5;

  std::ostringstream output{};
  zstream::zipstreambuf<BUF_SIZE> zsbuf{output, Z_BEST_COMPRESSION};
  std::ostream zip_output{&zsbuf};

  for (int i = 0; i < iterations; ++i) {
    zip_output << string_to_deflate;
  }
  zsbuf.zflush();

  REQUIRE(output.str().size() < string_to_deflate.size() * iterations);
}

TEST_CASE("Deflate streambuf no compression double flush", "[main]") {
  static constexpr std::string_view another_string_to_deflate =
      "second part of the party";

  std::ostringstream output;
  zstream::zipstreambuf<BUF_SIZE> zsbuf{output, Z_NO_COMPRESSION};
  std::ostream zip_output(&zsbuf);

  zip_output << string_to_deflate;
  zsbuf.zflush();
  auto found_idx = output.str().find(string_to_deflate);
  REQUIRE(found_idx != std::string::npos);

  zip_output << another_string_to_deflate;
  zsbuf.zflush();

  found_idx = output.str().find(another_string_to_deflate);
  REQUIRE(found_idx != std::string::npos);
}

// NOLINTNEXTLINE
TEMPLATE_TEST_CASE_SIG("Deflate streambuf no compression extremely small buffer",
                       "[main]",
                       ((zstream::buffer_size_t BUFSIZE), BUFSIZE),
                       13,
                       10,
                       1) {
  static constexpr auto iterations = 1000;

  std::ostringstream output;
  zstream::zipstreambuf<BUFSIZE> zsbuf{output, Z_NO_COMPRESSION};

  std::ostream zip_output(&zsbuf);
  for (int i = 0; i < iterations; ++i) {
    zip_output << string_to_deflate;
  }
  zsbuf.zflush();

  auto found_idx = output.str().find(string_to_deflate);
  REQUIRE(found_idx != std::string::npos);
}
