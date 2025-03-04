/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-03-04 09:24:11                                                

================================================================================*/

#include <string.h>

#include "common.h"
#include "deserializer.h"

static uint16_t deserialize_status_code(const char *buffer, const char *const buffer_end, http_response_t *const restrict response);
static uint16_t deserialize_reason_phrase(char *buffer, const char *const buffer_end, http_response_t *const restrict response);
static uint16_t deserialize_headers(char *restrict buffer, const char *const buffer_end, http_response_t *const restrict response);
static uint32_t atoui(const char *str, const char **const endptr);
static inline uint32_t mul10(uint32_t n);

uint32_t http1_deserialize(char *restrict buffer, const uint32_t buffer_size, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;
  const char *const buffer_end = buffer + buffer_size;

  bool error_occured = false;

  const uint16_t status_code_len = deserialize_status_code(buffer, buffer_end, response);
  error_occured |= (status_code_len == 0);
  buffer += status_code_len;

  const uint16_t reason_phrase_len = deserialize_reason_phrase(buffer, buffer_end, response);
  error_occured |= (reason_phrase_len == 0);
  buffer += reason_phrase_len;

  const uint16_t headers_len = deserialize_headers(buffer, buffer_end, response);
  error_occured |= (headers_len == 0);
  buffer += headers_len;

  response->body = (char *)((buffer < buffer_end) * (uintptr_t)buffer);

  return (buffer - buffer_start) * !error_occured;
}

static uint16_t deserialize_status_code(const char *buffer, const char *const buffer_end, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;
  
  const char *const space = memchr(buffer, ' ', buffer_end - buffer);
  
  const uint32_t status_code = atoui(space, &buffer);
  response->status_code = status_code;
  buffer++;

  const bool valid = (status_code - 100) < 499;

  return (buffer - buffer_start) * valid;
}

static uint16_t deserialize_reason_phrase(char *buffer, UNUSED const char *const buffer_end, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;

  buffer = rawmemchr(buffer, '\r');
  *buffer = '\0';

  uint32_t reason_phrase_len = buffer - buffer_start;

  response->reason_phrase = buffer_start;
  response->reason_phrase_len = reason_phrase_len;

  buffer += STR_LEN("\r\n");

  bool valid = (reason_phrase_len != 0);
  valid &= (reason_phrase_len <= UINT16_MAX);

  return (buffer - buffer_start) * valid;
}

static uint16_t deserialize_headers(char *restrict buffer, const char *const buffer_end, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;

  const uint16_t max_headers = response->headers_count;
  http_header_t *headers = response->headers;
  uint16_t headers_count = 0;

  while (LIKELY(*buffer != '\r'))
  {
    char *const key = buffer;
    buffer = memchr(buffer, ':', buffer_end - buffer);

    bool valid = (buffer != NULL) & (headers_count < max_headers);
    if (UNLIKELY(!valid))
      return 0;

    uint32_t key_len = buffer - key;
    *buffer++ = '\0';
    buffer += strspn(buffer, " \t");
    const bool valid_key = (key_len != 0) & (key_len <= UINT16_MAX);

    const char *const value = buffer;
    buffer = rawmemchr(buffer, '\r');

    uint32_t value_len = buffer - value;
    *buffer = '\0';
    buffer += STR_LEN("\r\n");
    const bool valid_value = (value_len != 0) & (value_len <= UINT16_MAX);

    valid = valid_key & valid_value;
    if (UNLIKELY(!valid))
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

static uint32_t atoui(const char *str, const char **const endptr)
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
    *endptr = str;

  return result;
}

static inline uint32_t mul10(uint32_t n)
{
  return (n << 3) + (n << 1);
}
