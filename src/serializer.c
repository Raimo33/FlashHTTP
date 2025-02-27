/*================================================================================

File: serializer.c                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-27 19:27:40                                                

================================================================================*/

#include "common.h"
#include "serializer.h"

#ifdef __AVX512F__
  //TODO define constants
#endif

#ifdef __AVX2__
  //TODO define constants
#endif

#ifdef __SSE2__
  //TODO define constants
#endif

CONSTRUCTOR void fh_serializer_init(void)
{
  //TODO initialize constants
}

inline static uint8_t serialize_method(char *restrict buffer, const http_method_t method);
static uint16_t serialize_path(char *restrict buffer, const char *restrict path, const uint8_t path_len);
inline static uint8_t serialize_version(char *restrict buffer, const http_version_t version);
static uint32_t serialize_headers(char *restrict buffer, const http_header_t *restrict headers, const uint8_t headers_count);
static uint32_t serialize_body(char *restrict buffer, const char *restrict body, const uint16_t body_len);

static const char methods_str[][sizeof(uint64_t)] ALIGNED(64) = {
  [GET] = "GET",
  [HEAD] = "HEAD",
  [POST] = "POST",
  [PUT] = "PUT",
  [DELETE] = "DELETE",
  [CONNECT] = "CONNECT",
  [OPTIONS] = "OPTIONS",
  [TRACE] = "TRACE",
  [PATCH] = "PATCH",
  [CONNECT] = "CONNECT"
};
static const uint8_t methods_len[] = {
  [GET] = STR_LEN("GET"),
  [HEAD] = STR_LEN("HEAD"),
  [POST] = STR_LEN("POST"),
  [PUT] = STR_LEN("PUT"),
  [DELETE] = STR_LEN("DELETE"),
  [CONNECT] = STR_LEN("CONNECT"),
  [OPTIONS] = STR_LEN("OPTIONS"),
  [TRACE] = STR_LEN("TRACE"),
  [PATCH] = STR_LEN("PATCH"),
  [CONNECT] = STR_LEN("CONNECT")
};

static const char versions_str[][sizeof(uint64_t)] ALIGNED(64) = {
  [HTTP_1_0] = "HTTP/1.0",
  [HTTP_1_1] = "HTTP/1.1"
  [HTTP_2_0] = "HTTP/2.0"
  [HTTP_3_0] = "HTTP/3.0"
};
static const uint8_t versions_len[] = {
  [HTTP_1_0] = STR_LEN("HTTP/1.0"),
  [HTTP_1_1] = STR_LEN("HTTP/1.1")
  [HTTP_2_0] = STR_LEN("HTTP/2.0")
  [HTTP_3_0] = STR_LEN("HTTP/3.0")
};

static const char clrf[sizeof(uint16_t)] = "\r\n";
static const char colon_space[sizeof(uint16_t)] = ": ";

//TODO explore (frame-based structure, header compression, multiplexing, QUIC, etc.)

uint32_t fh_serialize(const http_request_t *restrict request, char *restrict buffer)
{
  const char *buffer_start = buffer;

  buffer += serialize_method(buffer, request->method);
  buffer += serialize_path(buffer, request->path, request->path_len);
  buffer += serialize_version(buffer, request->version);
  buffer += serialize_headers(buffer, request->headers, request->headers_count);
  buffer += serialize_body(buffer, request->body, request->body_len);

  return buffer - buffer_start;
}

static inline uint8_t serialize_method(char *restrict buffer, const http_method_t method)
{
  const char *buffer_start = buffer;

  *(uint64_t *)buffer = *(uint64_t *)methods[method];
  buffer += methods_len[method];
  *buffer++ = ' ';
  
  return buffer - buffer_start;
}

static uint16_t serialize_path(char *restrict buffer, const char *restrict path, const uint8_t path_len)
{
  const char *buffer_start = buffer;

  memcpy(buffer, path, path_len);
  buffer += path_len;
  *buffer++ = ' ';
  
  return buffer - buffer_start;
}

static inline uint8_t serialize_version(char *restrict buffer, const http_version_t version)
{
  const char *buffer_start = buffer;

  *(uint64_t *)buffer = *(uint64_t *)versions_str[version];
  buffer += versions_len[version];
  *(uint16_t *)buffer = *(uint16_t *)clrf;
  buffer += sizeof(clrf);
  
  return buffer - buffer_start;
}

static uint32_t serialize_headers(char *restrict buffer, const http_header_t *restrict headers, const uint8_t headers_count)
{
  const char *buffer_start = buffer;

  for (uint8_t i = 0; LIKELY(i < headers_count); i++)
  {
    const http_header_t *header = &headers[i];

    memcpy(buffer, header->key, header->key_len);
    buffer += header->key_len;
    *(uint16_t *)buffer = *(uint16_t *)colon_space;
    buffer += sizeof(colon_space);

    memcpy(buffer, header->value, header->value_len);
    buffer += header->value_len;
    *(uint16_t *)buffer = *(uint16_t *)clrf;
    buffer += sizeof(clrf);
  }

  *(uint16_t *)buffer = *(uint16_t *)clrf;
  buffer += sizeof(clrf);

  return buffer - buffer_start;
}

static uint32_t serialize_body(char *restrict buffer, const char *restrict body, const uint16_t body_len)
{
  const char *buffer_start = buffer;

  memcpy(buffer, body, body_len);
  buffer += body_len;
  
  return buffer - buffer_start;
}
