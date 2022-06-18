#include <array>
#include <sstream>
#include <zlib.h>


template<size_t IN_BUFFER_SIZE, size_t OUT_BUFFER_SIZE>
class zipbuf : public std::streambuf {
public:
   using byte_type = Bytef;

   explicit zipbuf(std::ostream &out_stream, int compression_level = Z_DEFAULT_COMPRESSION)
      : out_stream_{out_stream}
      , compression_level_{compression_level}
      , was_deflate_init_{false}
   {
      in_buffer_prepare_for_input();
   }

   ~zipbuf() {
      deflate_end(deflate_state_);
   }

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
      if (overflow(traits_type::eof()) == traits_type::eof())
         return -1;
      return 0;
   }

   bool deflate_init(z_stream &stream, int compression_level) {
      if (!was_deflate_init_) {
         stream.zalloc = Z_NULL;
         stream.zfree = Z_NULL;
         stream.opaque = Z_NULL;
         deflateInit(&stream, compression_level);
         was_deflate_init_ = true;
         return true;
      }
      return false;
   }

   void deflate_end(z_stream &stream) {
      if (was_deflate_init_) {
         deflateEnd(&stream);
         was_deflate_init_ = false;
      }
   }

   bool check_out_buffer_overflow() {
      return deflate_state_.avail_out == 0;
   }

   bool check_out_buffer_not_empty() {
      return (OUT_BUFFER_BYTES - deflate_state_.avail_out) > 0;
   }

   bool input_buffer_deflate(int flush) {
      if (deflate_init(deflate_state_, compression_level)) {
         out_buffer_prepare_for_deflate(deflate_state_);
      }
      in_buffer_prepare_for_deflate(deflate_state_);

      int last_deflate_status_code{Z_OK};
      while (deflate_state_.avail_in > 0 && last_deflate_status_code == Z_OK) {
         last_deflate_status_code = deflate(&deflate_state_, flush);

         if (last_deflate_status_code == Z_OK) {
            if (check_out_buffer_overflow()) {
               out_buffer_write_to_ostream();
               out_buffer_prepare_for_deflate(deflate_state_);
            }
         }
      }

      if (check_out_buffer_not_empty()) {
         out_buffer_write_to_ostream();
         out_buffer_prepare_for_deflate(deflate_state_);
      }

      in_buffer_prepare_for_input();
      return true;
   }

   void in_buffer_prepare_for_input() {
      auto const in_begin = in_buffer_.data();
      setp(in_begin, in_begin + in_buffer_.size());
   }

   void in_buffer_prepare_for_deflate(z_stream &stream) {
      stream.next_in = reinterpret_cast<byte_type*>(pbase());
      stream.avail_in = static_cast<uInt>(
         (pptr() - pbase()) * CHAR_TYPE_SIZE
      );
   }

   auto get_out_buffer_bytes_count() {
      return OUT_BUFFER_BYTES - deflate_state_.avail_out;
   }

   void out_buffer_prepare_for_deflate(z_stream &stream) {
      auto [full_chars, remainder] = std::lldiv(
         get_out_buffer_bytes_count(),
         CHAR_TYPE_SIZE
      );
      // copy not full char of last deflate to begin of buffer
      if (remainder) {
         out_buffer_[0] = out_buffer_[full_chars];
      }
      stream.next_out = reinterpret_cast<byte_type*>(out_buffer_.data()) + remainder;
      stream.avail_out = static_cast<uInt>(OUT_BUFFER_BYTES);
   }

   void out_buffer_write_to_ostream() {
      auto const out_buffer_chars = static_cast<std::streamsize>(
         get_out_buffer_bytes_count() / CHAR_TYPE_SIZE
      );
      out_stream_.write(out_buffer_.data(), out_buffer_chars);
   }

   void zflush() {
      sync();
      input_buffer_deflate(Z_FINISH);
      out_stream_.flush();
      deflate_end(deflate_state_);
   }

public:
   std::array<char_type, IN_BUFFER_SIZE> in_buffer_;
   std::array<char_type, OUT_BUFFER_SIZE> out_buffer_;
   z_stream deflate_state_;
   std::ostream &out_stream_;
   bool was_deflate_init_;
   int compression_level_;

   static constexpr auto CHAR_TYPE_SIZE{sizeof(char_type)};
   static constexpr auto IN_BUFFER_BYTES = IN_BUFFER_SIZE * CHAR_TYPE_SIZE;
   static constexpr auto OUT_BUFFER_BYTES = OUT_BUFFER_SIZE * CHAR_TYPE_SIZE;
};
