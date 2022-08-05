#ifndef STREAM_ZIP_DEFLATE_STREAMBUF_HPP
#define STREAM_ZIP_DEFLATE_STREAMBUF_HPP

#include <zconf.h>
#include <zlib.h>

#include <array>
#include <limits>
#include <sstream>
#include <streambuf>

namespace zstream {

using buffer_size_t = uInt;

template <buffer_size_t BUFFER_SIZE>
class zipstreambuf final : public std::streambuf {
  using byte_type = Bytef;
  static_assert(BUFFER_SIZE > 0, "BUFFER_SIZE size must be larger than 0");

 public:
  static constexpr auto CHAR_TYPE_SIZE = sizeof(char_type);
  static constexpr auto OUT_BUFFER_TOTAL_BYTES = BUFFER_SIZE * CHAR_TYPE_SIZE;

  explicit zipstreambuf(std::ostream& out_stream,
                        int compression_level = Z_DEFAULT_COMPRESSION);
  zipstreambuf(zipstreambuf const& orig) = delete;
  zipstreambuf(zipstreambuf&& orig) = delete;
  auto operator=(zipstreambuf const& orig) -> zipstreambuf& = delete;
  auto operator=(zipstreambuf&& orig) -> zipstreambuf& = delete;
  ~zipstreambuf() override;

  void zflush();

 protected:
  auto overflow(int_type chr) -> int_type override;
  auto sync() -> int_type override;

 private:
  auto deflate_init() -> bool;
  void deflate_end();

  [[nodiscard]] auto check_out_buffer_not_empty() const -> bool;
  [[nodiscard]] auto check_out_buffer_overflow() const -> bool;

  auto input_buffer_deflate(int flush) -> bool;

  void in_buffer_prepare_for_deflate();
  void in_buffer_prepare_for_input();

  [[nodiscard]] auto get_out_buffer_bytes_count() const -> buffer_size_t;
  void out_buffer_prepare_for_deflate();
  void out_buffer_write_to_ostream();
  void out_buffer_write_remainder_to_ostream();

  std::array<char_type, BUFFER_SIZE> in_buffer_;
  std::array<char_type, BUFFER_SIZE> out_buffer_;
  z_stream deflate_state_ = {};
  std::ostream& out_stream_;
  bool was_deflate_init_ = false;
  int compression_level_;
};

template <buffer_size_t BUFFER_SIZE>
zipstreambuf<BUFFER_SIZE>::zipstreambuf(std::ostream& out_stream, int compression_level)
    : out_stream_{out_stream}, compression_level_{compression_level} {
  in_buffer_prepare_for_input();
}

template <buffer_size_t BUFFER_SIZE>
zipstreambuf<BUFFER_SIZE>::~zipstreambuf() {
  deflate_end();
}

template <buffer_size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::zflush() {
  sync();
  input_buffer_deflate(Z_FINISH);
  out_buffer_write_remainder_to_ostream();
  out_stream_.flush();
  deflate_end();
}

template <buffer_size_t BUFFER_SIZE>
auto zipstreambuf<BUFFER_SIZE>::overflow(int_type chr) -> std::streambuf::int_type {
  if (pptr() > pbase()) {
    if (!input_buffer_deflate(Z_NO_FLUSH)) {
      return traits_type::eof();
    }
  }
  // write c if not EOF
  if (!traits_type::eq_int_type(chr, traits_type::eof())) {
    *pptr() = traits_type::to_char_type(chr);
    pbump(1);
    return chr;
  }
  return traits_type::eof();
}

template <buffer_size_t BUFFER_SIZE>
auto zipstreambuf<BUFFER_SIZE>::sync() -> std::streambuf::int_type {
  if (overflow(traits_type::eof()) == traits_type::eof()) {
    return -1;
  }
  return 0;
}

template <buffer_size_t BUFFER_SIZE>
auto zipstreambuf<BUFFER_SIZE>::deflate_init() -> bool {
  if (!was_deflate_init_) {
    deflate_state_.zalloc = Z_NULL;
    deflate_state_.zfree = Z_NULL;
    deflate_state_.opaque = Z_NULL;
    deflateInit(&deflate_state_, compression_level_);

    deflate_state_.next_in = nullptr;
    deflate_state_.avail_in = 0;
    deflate_state_.next_out = nullptr;
    deflate_state_.avail_out = 0;

    was_deflate_init_ = true;
    return true;
  }
  return false;
}

template <buffer_size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::deflate_end() {
  if (was_deflate_init_) {
    deflateEnd(&deflate_state_);
    was_deflate_init_ = false;
  }
}

template <buffer_size_t BUFFER_SIZE>
auto zipstreambuf<BUFFER_SIZE>::check_out_buffer_not_empty() const -> bool {
  return get_out_buffer_bytes_count() > 0;
}

template <buffer_size_t BUFFER_SIZE>
auto zipstreambuf<BUFFER_SIZE>::check_out_buffer_overflow() const -> bool {
  return deflate_state_.avail_out == 0;
}

template <buffer_size_t BUFFER_SIZE>
auto zipstreambuf<BUFFER_SIZE>::input_buffer_deflate(int flush) -> bool {
  if (deflate_init()) {
    out_buffer_prepare_for_deflate();
  }
  in_buffer_prepare_for_deflate();

  int last_deflate_status_code = Z_OK;
  while (((flush != Z_NO_FLUSH) || deflate_state_.avail_in > 0) &&
         last_deflate_status_code == Z_OK) {
    last_deflate_status_code = deflate(&deflate_state_, flush);

    if (last_deflate_status_code == Z_OK) {
      if (check_out_buffer_overflow()) {
        out_buffer_write_to_ostream();
        out_buffer_prepare_for_deflate();
      }
    }
  }

  if (check_out_buffer_not_empty()) {
    out_buffer_write_to_ostream();
    out_buffer_prepare_for_deflate();
  }

  in_buffer_prepare_for_input();
  return true;
}

template <buffer_size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::in_buffer_prepare_for_deflate() {
  deflate_state_.next_in = static_cast<byte_type*>(static_cast<void*>(pbase()));
  assert((pptr() - pbase()) < std::numeric_limits<buffer_size_t>::max() &&
         "Buffer can not be larger than max of buffer_size_t");
  deflate_state_.avail_in =
      static_cast<buffer_size_t>((pptr() - pbase()) * CHAR_TYPE_SIZE);
}

template <buffer_size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::in_buffer_prepare_for_input() {
  auto const in_begin = in_buffer_.data();
  setp(in_begin, in_begin + in_buffer_.size());
}

template <buffer_size_t BUFFER_SIZE>
auto zipstreambuf<BUFFER_SIZE>::get_out_buffer_bytes_count() const -> buffer_size_t {
  if (deflate_state_.next_out != nullptr) {
    return OUT_BUFFER_TOTAL_BYTES - deflate_state_.avail_out;
  }
  return 0;
}

template <buffer_size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::out_buffer_prepare_for_deflate() {
  /**
   * If number of resulting bytes in output
   * buffer is not a multiple of char_type
   * then remainder may occur after writing
   * whole chars to out stream. So we need
   * to place this remainder to the
   * beginning of the buffer.
   */
  auto const [full_chars, remainder] =
      std::lldiv(get_out_buffer_bytes_count(), CHAR_TYPE_SIZE);
  if (remainder != 0) {
    out_buffer_[0] = out_buffer_[full_chars];
  }
  auto* const casted_buffer =
      static_cast<byte_type*>(static_cast<void*>(out_buffer_.data()));
  deflate_state_.next_out = casted_buffer + remainder;
  deflate_state_.avail_out = OUT_BUFFER_TOTAL_BYTES - remainder;
}

template <buffer_size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::out_buffer_write_to_ostream() {
  auto const out_buffer_chars =
      static_cast<std::streamsize>(get_out_buffer_bytes_count() / CHAR_TYPE_SIZE);
  out_stream_.write(out_buffer_.data(), out_buffer_chars);
}

template <buffer_size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::out_buffer_write_remainder_to_ostream() {
  // It would be great to zero bytes that are not overlapped with the
  // remainder
  if (get_out_buffer_bytes_count()) {
    out_stream_.write(out_buffer_.data(), 1);
  }
}

}  // namespace zstream

#endif  // STREAM_ZIP_DEFLATE_STREAMBUF_HPP
