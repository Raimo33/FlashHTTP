/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-03-05 21:04:20                                                

================================================================================*/

#include <string.h>

#include "common.h"
#include "deserializer.h"

static uint32_t deserialize_status_line(char *buffer, char *const buffer_end, http_response_t *const restrict response);
static uint16_t deserialize_status_code(char *buffer, char *const line_end, http_response_t *const restrict response);
static uint16_t deserialize_reason_phrase(char *buffer, char *const line_end, http_response_t *const restrict response);
static uint32_t deserialize_headers(char *restrict buffer, char *const buffer_end, http_response_t *const restrict response);
static uint32_t atoui(const char *str, char **endptr);
static inline uint32_t mul10(uint32_t n);

uint32_t http1_deserialize(char *restrict buffer, const uint32_t buffer_size, http_response_t *const restrict response)
{
  char *const buffer_start = buffer;
  char *const buffer_end = buffer + buffer_size;

  uint32_t parsed_bytes;

  parsed_bytes = deserialize_status_line(buffer, buffer_end, response);
  if (UNLIKELY(parsed_bytes == 0))
    return 0;
  buffer += parsed_bytes;

  parsed_bytes = deserialize_headers(buffer, buffer_end, response);
  if (UNLIKELY(parsed_bytes == 0))
    return 0;
  buffer += parsed_bytes;

  response->body = (char *)((buffer < buffer_end) * (uintptr_t)buffer);

  return buffer - buffer_start;
}

static uint32_t deserialize_status_line(char *buffer, char *const buffer_end, http_response_t *const restrict response)
{
  char *const line_start = buffer;
  char *const line_end = memmem(buffer, buffer_end - buffer, "\r\n", STR_LEN("\r\n"));

  uint32_t parsed_bytes;

  parsed_bytes = deserialize_status_code(buffer, line_end, response);
  if (UNLIKELY(parsed_bytes == 0))
    return 0;
  buffer += parsed_bytes;

  parsed_bytes = deserialize_reason_phrase(buffer, line_end, response);
  if (UNLIKELY(parsed_bytes == 0))
    return 0;
  buffer += parsed_bytes;

  return buffer - line_start;
}

static uint16_t deserialize_status_code(char *buffer, char *const line_end, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;
  const char *const space = memchr(buffer, ' ', line_end - buffer);

  const uint32_t status_code = atoui(space, &buffer);
  response->status_code = status_code;

  const bool valid = (status_code - 100) < 499;

  return (buffer - buffer_start) * valid;
}

static uint16_t deserialize_reason_phrase(char *buffer, char *const line_end, http_response_t *const restrict response)
{
  char *const buffer_start = buffer;

  buffer += strspn(buffer, " \t");
  char *const reason_phrase_start = buffer;
  buffer = line_end;
  *buffer = '\0';

  uint32_t reason_phrase_len = buffer - reason_phrase_start;

  response->reason_phrase = reason_phrase_start;
  response->reason_phrase_len = reason_phrase_len;

  buffer += STR_LEN("\r\n");

  bool valid = (reason_phrase_len != 0);
  valid &= (reason_phrase_len <= UINT16_MAX);

  return (buffer - buffer_start) * valid;
}

static uint32_t deserialize_headers(char *restrict buffer, char *const buffer_end, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;

  const uint16_t max_headers = response->headers_count;
  http_header_t *headers = response->headers;
  uint16_t headers_count = 0;

  while (LIKELY(!memcmp2(buffer, "\r\n")))
  {
    char *const key = buffer;
    buffer = memchr(buffer, ':', buffer_end - buffer);
    bool valid_header = (buffer != NULL) & (headers_count < max_headers);
    if (UNLIKELY(!valid_header))
      return 0;
    uint32_t key_len = buffer - key;
    *buffer++ = '\0';
    buffer += strspn(buffer, " \t");
    const bool valid_key = (key_len != 0) & (key_len <= UINT16_MAX);

    char *const value = buffer;
    buffer = memmem(buffer, buffer_end - buffer, "\r\n", STR_LEN("\r\n"));
    if (UNLIKELY(buffer == NULL))
      return 0;
    uint32_t value_len = buffer - value;
    *buffer = '\0';
    buffer += STR_LEN("\r\n");
    const bool valid_value = (value_len != 0) & (value_len <= UINT16_MAX);

    valid_header = valid_key & valid_value;
    if (UNLIKELY(!valid_header))
      return 0;

    *headers++ = (http_header_t) {
      .key = key,
      .key_len = key_len,
      .value = value,
      .value_len = value_len
    };
    headers_count++;
  }

  buffer += STR_LEN("\r\n");
  response->headers_count = headers_count;

  return buffer - buffer_start;
}

static uint32_t atoui(const char *str, char **endptr)
{
  uint32_t result = 0;

  if (UNLIKELY(!str))
    return 0;

  str += strspn(str, " \t\n\r\v\f");

  while (LIKELY(((uint8_t)(*str - '0') < 10)))
  {
    result = mul10(result) + (*str - '0');
    str++;
  }

  if (LIKELY(endptr)) //TODO branchless
    *endptr = (char *)str;

  return result;
}

static inline uint32_t mul10(uint32_t n)
{
  return (n << 3) + (n << 1);
}
