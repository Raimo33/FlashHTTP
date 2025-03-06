# Installation Guide

To install FlashHTTP you must compile it from source, as that will guarantee the best performance.

## Requirements

  - CMake 3.10 or later
  - gcc with c23 support
  - CPU with misaligned memory access support

## Building

  - Clone the repository: ```git clone https://github.com/Raimo33/FlashHTTP.git``` or download the source code from the [release page](https://github.com/Raimo33/FlashHTTP/releases)
  - Generate the build files: ```cmake .```
  - Build the library: ```cmake --build . --parallel```
  - Optionally install the library: ```cmake --install .```

## Testing
  
  - Compile the tests: ```cmake --build . --parallel --target test```
  - Run the test executable: ```./test```

in case of failure, please open an issue on [GitHub](https://github.com/Raimo33/FlashHTTP/labels/test-failed) if there isn't one already.