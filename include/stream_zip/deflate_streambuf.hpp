#ifndef DEFLATE_STREAMBUF_HPP
#define DEFLATE_STREAMBUF_HPP

#include <array>
#include <sstream>
#include <zlib.h>


namespace zstream {

template<size_t IN_BUFFER_SIZE, size_t OUT_BUFFER_SIZE>
class zipstreambuf : public std::streambuf {
public:
   using byte_type = Bytef;

   explicit zipstreambuf(std::ostream &out_stream, int compression_level = Z_DEFAULT_COMPRESSION)
      : out_stream_{out_stream},
        compression_level_{compression_level},
        was_deflate_init_{false}
   {
      in_buffer_prepare_for_input();
   }

   ~zipstreambuf() {
      deflate_end();
   }

   void zflush() {
      sync();
      input_buffer_deflate(Z_FINISH);
      out_stream_.flush();
      deflate_end();
   }

protected:
   int_type overflow(int_type c) override {
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

   int_type sync() override {
      auto const c = overflow(traits_type::eof());
      if (c == traits_type::eof())
         return -1;
      return 0;
   }

private:
   bool deflate_init() {
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

   void deflate_end() {
      if (was_deflate_init_) {
         deflateEnd(&deflate_state_);
         was_deflate_init_ = false;
      }
   }

   bool check_out_buffer_overflow() {
      return deflate_state_.avail_out == 0;
   }

   bool check_out_buffer_not_empty() {
      return (OUT_BUFFER_TOTAL_BYTES - deflate_state_.avail_out) > 0;
   }

   bool input_buffer_deflate(int flush) {
      if (deflate_init()) {
         out_buffer_prepare_for_deflate();
      }
      in_buffer_prepare_for_deflate();

      int last_deflate_status_code{Z_OK};
      while ((flush || deflate_state_.avail_in > 0)
             && last_deflate_status_code == Z_OK)
      {
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

   void in_buffer_prepare_for_input() {
      auto const in_begin = in_buffer_.data();
      setp(in_begin, in_begin + in_buffer_.size());
   }

   void in_buffer_prepare_for_deflate() {
      deflate_state_.next_in = reinterpret_cast<byte_type*>(pbase());
      deflate_state_.avail_in = static_cast<uInt>(
         (pptr() - pbase()) * CHAR_TYPE_SIZE
      );
   }

   size_t get_out_buffer_bytes_count() {
      if (deflate_state_.next_out != nullptr) {
         return OUT_BUFFER_TOTAL_BYTES - deflate_state_.avail_out;
      }
      return 0;
   }

   void out_buffer_prepare_for_deflate() {
      /**
       * If number of resulting bytes in output buffer is not a
       * multiple of char_type then remainder may occur after
       * writing whole chars to out stream.
       * So we need to place this remainder to the beginning of the buffer.
       */
      auto [full_chars, remainder] = std::lldiv(
         get_out_buffer_bytes_count(),
         CHAR_TYPE_SIZE
      );
      if (remainder != 0) {
         out_buffer_[0] = out_buffer_[full_chars];
      }
      deflate_state_.next_out = reinterpret_cast<byte_type*>(out_buffer_.data()) + remainder;
      deflate_state_.avail_out = static_cast<uInt>(OUT_BUFFER_TOTAL_BYTES - remainder);
   }

   void out_buffer_write_to_ostream() {
      auto const out_buffer_chars = static_cast<std::streamsize>(
         get_out_buffer_bytes_count() / CHAR_TYPE_SIZE
      );
      out_stream_.write(out_buffer_.data(), out_buffer_chars);
   }

private:
   std::array<char_type, IN_BUFFER_SIZE> in_buffer_;
   std::array<char_type, OUT_BUFFER_SIZE> out_buffer_;
   z_stream deflate_state_;
   std::ostream &out_stream_;
   bool was_deflate_init_;
   int compression_level_;

   static constexpr auto CHAR_TYPE_SIZE{sizeof(char_type)};
   static constexpr auto OUT_BUFFER_TOTAL_BYTES = OUT_BUFFER_SIZE * CHAR_TYPE_SIZE;
};

}

#endif // DEFLATE_STREAMBUF_HPP
