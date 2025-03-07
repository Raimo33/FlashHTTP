cmake_minimum_required(VERSION 3.5)
project(flashhttp VERSION 1.4.1 LANGUAGES C)

set(COMMON_COMPILE_OPTIONS
  -Wall
  -Wextra
  -Wpedantic
  -O3
  -march=native
  -lto
)

set(COMMON_COMPILE_DEFINITIONS _GNU_SOURCE)

add_library(flashhttp_shared SHARED)
add_library(flashhttp_static STATIC)
add_library(flashhttp ALIAS flashhttp_shared)

foreach(TARGET flashhttp_shared flashhttp_static)
  target_sources(${TARGET}
    PRIVATE
      src/deserializer.c
      src/serializer.c
      src/common.c
    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS include
      FILES
        include/flashhttp.h
        include/deserializer.h
        include/serializer.h
        include/structs.h
  )

  set_target_properties(${TARGET} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME flashhttp
    C_STANDARD 23
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
  )
endforeach()

add_executable(test tests/test.c)
target_link_libraries(test PRIVATE flashhttp_static)

add_executable(benchmark benchmarks/benchmark.c)
target_link_libraries(benchmark PRIVATE flashhttp_static m)

foreach(TARGET test benchmark)
  set_target_properties(${TARGET} PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
  )
endforeach()

foreach(TARGET flashhttp_shared flashhttp_static test benchmark)
  target_compile_options(${TARGET} PRIVATE ${COMMON_COMPILE_OPTIONS})
  target_compile_definitions(${TARGET} PRIVATE ${COMMON_COMPILE_DEFINITIONS})
endforeach()

install(TARGETS flashhttp_shared flashhttp_static
  EXPORT flashhttp-targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  FILE_SET HEADERS DESTINATION include/flashhttp
)