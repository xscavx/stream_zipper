#ifndef DEFLATE_STREAMBUF_HPP
#define DEFLATE_STREAMBUF_HPP
#include <array>
#include <sstream>
#include <zlib.h>

namespace zstream {

template <size_t BUFFER_SIZE> class zipstreambuf final : public std::streambuf {
public:
  using byte_type = Bytef;

  zipstreambuf(std::ostream &out_stream,
               int compression_level = Z_DEFAULT_COMPRESSION);
  virtual ~zipstreambuf();
  zipstreambuf(zipstreambuf const &orig) = delete;
  zipstreambuf(zipstreambuf &&orig) = delete;
  zipstreambuf &operator=(zipstreambuf const &orig) = delete;
  zipstreambuf &operator=(zipstreambuf &&orig) = delete;

  void zflush();

protected:
  virtual int_type overflow(int_type c) override;
  virtual int_type sync() override;

private:
  bool deflate_init();
  void deflate_end();

  bool check_out_buffer_not_empty();
  bool check_out_buffer_overflow();

  bool input_buffer_deflate(int flush);

  void in_buffer_prepare_for_deflate();
  void in_buffer_prepare_for_input();

  size_t get_out_buffer_bytes_count();
  void out_buffer_prepare_for_deflate();
  void out_buffer_write_to_ostream();
  void out_buffer_write_remainder_to_ostream();

  std::array<char_type, BUFFER_SIZE> in_buffer_;
  std::array<char_type, BUFFER_SIZE> out_buffer_;
  z_stream deflate_state_;
  std::ostream &out_stream_;
  bool was_deflate_init_;
  int compression_level_;

  static constexpr auto CHAR_TYPE_SIZE{sizeof(char_type)};
  static constexpr auto OUT_BUFFER_TOTAL_BYTES = BUFFER_SIZE * CHAR_TYPE_SIZE;
};

template <size_t BUFFER_SIZE>
zipstreambuf<BUFFER_SIZE>::zipstreambuf(std::ostream &out_stream,
                                        int compression_level)
    : out_stream_{out_stream}, compression_level_{compression_level},
      was_deflate_init_{false} {
  in_buffer_prepare_for_input();
}

template <size_t BUFFER_SIZE> zipstreambuf<BUFFER_SIZE>::~zipstreambuf() {
  deflate_end();
}

template <size_t BUFFER_SIZE> void zipstreambuf<BUFFER_SIZE>::zflush() {
  sync();
  input_buffer_deflate(Z_FINISH);
  out_buffer_write_remainder_to_ostream();
  out_stream_.flush();
  deflate_end();
}

template <size_t BUFFER_SIZE>
std::streambuf::int_type zipstreambuf<BUFFER_SIZE>::overflow(int_type c) {
  if (pptr() > pbase()) {
    if (!input_buffer_deflate(Z_NO_FLUSH))
      return traits_type::eof();
  }

  // write c if not EOF
  if (traits_type::not_eof(c)) {
    *pptr() = traits_type::to_char_type(c);
    pbump(1);
    return c;
  }

  return traits_type::eof();
}

template <size_t BUFFER_SIZE>
std::streambuf::int_type zipstreambuf<BUFFER_SIZE>::sync() {
  auto const c = overflow(traits_type::eof());
  if (c == traits_type::eof())
    return -1;
  return 0;
}

template <size_t BUFFER_SIZE> bool zipstreambuf<BUFFER_SIZE>::deflate_init() {
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

template <size_t BUFFER_SIZE> void zipstreambuf<BUFFER_SIZE>::deflate_end() {
  if (was_deflate_init_) {
    deflateEnd(&deflate_state_);
    was_deflate_init_ = false;
  }
}

template <size_t BUFFER_SIZE>
bool zipstreambuf<BUFFER_SIZE>::check_out_buffer_not_empty() {
  return (OUT_BUFFER_TOTAL_BYTES - deflate_state_.avail_out) > 0;
}

template <size_t BUFFER_SIZE>
bool zipstreambuf<BUFFER_SIZE>::check_out_buffer_overflow() {
  return deflate_state_.avail_out == 0;
}

template <size_t BUFFER_SIZE>
bool zipstreambuf<BUFFER_SIZE>::input_buffer_deflate(int flush) {
  if (deflate_init()) {
    out_buffer_prepare_for_deflate();
  }
  in_buffer_prepare_for_deflate();

  int last_deflate_status_code{Z_OK};
  while ((flush || deflate_state_.avail_in > 0) &&
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

template <size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::in_buffer_prepare_for_deflate() {
  deflate_state_.next_in = reinterpret_cast<byte_type *>(pbase());
  deflate_state_.avail_in =
      static_cast<uInt>((pptr() - pbase()) * CHAR_TYPE_SIZE);
}

template <size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::in_buffer_prepare_for_input() {
  auto const in_begin = in_buffer_.data();
  setp(in_begin, in_begin + in_buffer_.size());
}

template <size_t BUFFER_SIZE>
size_t zipstreambuf<BUFFER_SIZE>::get_out_buffer_bytes_count() {
  if (deflate_state_.next_out != nullptr) {
    return OUT_BUFFER_TOTAL_BYTES - deflate_state_.avail_out;
  }
  return 0;
}

template <size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::out_buffer_prepare_for_deflate() {
  /**
   * If number of resulting bytes in output
   * buffer is not a multiple of char_type
   * then remainder may occur after writing
   * whole chars to out stream. So we need
   * to place this remainder to the
   * beginning of the buffer.
   */
  auto [full_chars, remainder] =
      std::lldiv(get_out_buffer_bytes_count(), CHAR_TYPE_SIZE);
  if (remainder != 0) {
    out_buffer_[0] = out_buffer_[full_chars];
  }
  deflate_state_.next_out =
      reinterpret_cast<byte_type *>(out_buffer_.data()) + remainder;
  deflate_state_.avail_out =
      static_cast<uInt>(OUT_BUFFER_TOTAL_BYTES - remainder);
}

template <size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::out_buffer_write_to_ostream() {
  auto const out_buffer_chars = static_cast<std::streamsize>(
      get_out_buffer_bytes_count() / CHAR_TYPE_SIZE);
  out_stream_.write(out_buffer_.data(), out_buffer_chars);
}

template <size_t BUFFER_SIZE>
void zipstreambuf<BUFFER_SIZE>::out_buffer_write_remainder_to_ostream() {
  // It would be great to zero bytes that are not overlapped with the remainder
  if (get_out_buffer_bytes_count()) {
    out_stream_.write(out_buffer_.data(), 1);
  }
}

} // namespace zstream

#endif // DEFLATE_STREAMBUF_HPP
