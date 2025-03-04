cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER gcc)
project(flashttp LANGUAGES C VERSION 1.0.0)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(FLASHTTP_VERSION_MAJOR 1)
set(FLASHTTP_VERSION_MINOR 0)
set(FLASHTTP_VERSION_PATCH 0)
set(FLASHTTP_VERSION "${FLASHTTP_VERSION_MAJOR}.${FLASHTTP_VERSION_MINOR}.${FLASHTTP_VERSION_PATCH}")

set(FLASHTTP_SOURCES
  src/deserializer.c
  src/serializer.c
  src/common.c
)

set(FLASHTTP_PUBLIC_HEADERS
  include/flashttp.h
  include/deserializer.h
  include/serializer.h
  include/structs.h
)

set(FLASHTTP_COMPILE_OPTIONS
  -Wall -Wextra -Werror -Wpedantic
  -fwrapv -fno-strict-aliasing -Ofast -march=native -flto
  -fno-semantic-interposition -fno-stack-protector -fno-plt 
  -fno-asynchronous-unwind-tables -funroll-loops -fomit-frame-pointer
)

add_library(flashttp_shared SHARED ${FLASHTTP_SOURCES})
set_target_properties(flashttp_shared PROPERTIES
  VERSION ${FLASHTTP_VERSION}
  SOVERSION ${FLASHTTP_VERSION_MAJOR}
  OUTPUT_NAME flashttp
)
target_compile_options(flashttp_shared PUBLIC ${FLASHTTP_COMPILE_OPTIONS})
target_include_directories(flashttp_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(flashttp_shared PUBLIC _GNU_SOURCE)

add_library(flashttp_static STATIC ${FLASHTTP_SOURCES})
set_target_properties(flashttp_static PROPERTIES OUTPUT_NAME flashttp)
target_compile_options(flashttp_static PUBLIC ${FLASHTTP_COMPILE_OPTIONS})
target_include_directories(flashttp_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(flashttp_static PUBLIC _GNU_SOURCE)

install(TARGETS flashttp_shared flashttp_static
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

install(FILES ${FLASHTTP_PUBLIC_HEADERS} DESTINATION include/flashttp)

add_executable(test EXCLUDE_FROM_ALL tests/test.c)
target_link_libraries(test flashttp_static)
target_compile_options(test PUBLIC ${FLASHTTP_COMPILE_OPTIONS} -flto)
target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(test PUBLIC _GNU_SOURCE)

add_executable(benchmark EXCLUDE_FROM_ALL benchmarks/benchmark.c)
target_link_libraries(benchmark flashttp_static m)
target_compile_options(benchmark PUBLIC ${FLASHTTP_COMPILE_OPTIONS} -flto)
target_include_directories(benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(benchmark PUBLIC _GNU_SOURCE)