cmake_minimum_required(VERSION 3.23)
project(flashhttp VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(FLASHHTTP_COMPILE_OPTIONS
  -Wall -Wextra -Wpedantic
  -fwrapv -fno-strict-aliasing -O3 -march=native -flto
  -fno-semantic-interposition -fno-stack-protector -fno-plt
  -fno-asynchronous-unwind-tables -funroll-loops -fomit-frame-pointer
)

set(SOURCES
  src/deserializer.c
  src/serializer.c
  src/common.c
)

set(HEADERS
  include/flashhttp.h
  include/deserializer.h
  include/serializer.h
  include/structs.h
)

add_library(flashhttp_shared SHARED ${SOURCES} ${HEADERS})
add_library(flashhttp_static STATIC ${SOURCES} ${HEADERS})

foreach(TARGET flashhttp_shared flashhttp_static)
  set_target_properties(${TARGET} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME flashhttp
  )
  target_include_directories(${TARGET} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/flashhttp>
  )
  target_compile_options(${TARGET} PRIVATE ${FLASHHTTP_COMPILE_OPTIONS})
  target_compile_definitions(${TARGET} PRIVATE _GNU_SOURCE)
endforeach()

install(TARGETS flashhttp_shared flashhttp_static
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include/flashhttp)

add_executable(test tests/test.c)
target_link_libraries(test PRIVATE flashhttp_static)
target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(test PRIVATE ${FLASHHTTP_COMPILE_OPTIONS})
target_compile_definitions(test PRIVATE _GNU_SOURCE)

add_executable(benchmark benchmarks/benchmark.c)
target_link_libraries(benchmark PRIVATE flashhttp_static m)
target_include_directories(benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(benchmark PRIVATE ${FLASHHTTP_COMPILE_OPTIONS})
target_compile_definitions(benchmark PRIVATE _GNU_SOURCE)