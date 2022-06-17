#include <iostream>
#include "deflate_streambuf.hxx"


int compress() {
   std::string string_in{"test string very large wowow, stp it please, please please test string very large wowow, stp it please, please please"};
   std::istringstream stringstream_in{string_in};

   zipbuf<CHUNK_SIZE> zbuf{std::cout};

   return 1;
}

int main() {
   compress();
   return 0;
}
