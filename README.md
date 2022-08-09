# libyaz0

A very fast compressor/decompressor for the Yaz0 format, using a fixed amount of memory.

## Build

    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build .

## Implementation

It is very hard to make a fast yaz0 compressor due to it's design.  
There is a sliding window to search for matching pattern, and a decent compressor
needs either to backtrack (extremely impractical) or at least to look-ahead to get good
compression ratios.

This implementation uses an open-addressing multi hash table to store the previous patterns.  
A bunch of other optimizations like hash rebuild and pessimistic match checks are applied
to make it faster.  

## License

This software is available under the [MIT license](LICENSE).

## Author

This software was written by [Maxime Bacoux "Nax"](https://github.com/Nax).
