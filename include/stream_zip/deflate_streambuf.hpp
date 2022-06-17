#include <array>
#include <sstream>
#include <zlib.h>


inline constexpr auto CHUNK_SIZE = 10;


template<int BUFSIZE>
class zipbuf : public std::streambuf {
public:
   explicit zipbuf(std::ostream &out_stream, int compression_level = Z_DEFAULT_COMPRESSION)
      : out_stream_(out_stream)
   {
      init_deflate_state(compression_level);
      init_buffers_state();
   }

   ~zipbuf() {
      deflateEnd(&stream_state_);
   }

   void init_deflate_state(int compression_level) {
      stream_state_.zalloc = Z_NULL;
      stream_state_.zfree = Z_NULL;
      stream_state_.opaque = Z_NULL;
      deflateInit(&stream_state_, compression_level);
   }

   void init_buffers_state() {
      stream_state_.next_out = reinterpret_cast<Bytef*>(out_buffer_.data());
      stream_state_.avail_out = static_cast<uInt>(out_buffer_.size());
      
      auto out_begin = out_buffer_.data();
      auto out_end = out_begin + static_cast<uInt>(out_buffer_.size());
      setp(out_begin, out_end);
   }

   int_type overflow(int_type c) override {
      if (pptr() > pbase()) {
         if (!zip_buffer())
            return traits_type::eof();

         // pptr() automamtically sets to pbase()
         setp(pbase(), epptr());
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

   bool zip_buffer() {
      stream_state_.next_in = reinterpret_cast<Bytef*>(pbase());
      // input buffer available bytes
      stream_state_.avail_in =  (pptr() - pbase()) * sizeof(char_type);

      int deflate_state{Z_OK};
      while (stream_state_.avail_in > 0 && deflate_state == Z_OK) {
         deflate_state = deflate(&stream_state_, Z_NO_FLUSH);
      }
      return true;
   }

public:
   std::array<char_type, BUFSIZE> in_buffer_{}, out_buffer_{};
   z_stream stream_state_{};
   std::ostream &out_stream_;
   std::istream *in_stream_;
};
