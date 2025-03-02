/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-03-02 14:17:30                                                

================================================================================*/

#include <string.h>
#include <xxhash.h>

#include "common.h"
#include "deserializer.h"

static uint16_t deserialize_status_code(const char *buffer, http_response_t *const restrict response);
static uint16_t deserialize_reason_phrase(char *buffer, http_response_t *const restrict response);
static uint16_t deserialize_headers(char *restrict buffer, http_response_t *const restrict response);
static void header_map_set(http_header_map_t *const restrict map, const http_header_t *const restrict header);
static void strtolower(char *str, uint16_t len);
static uint32_t atoui(const char *str, const char **const endptr);
static inline uint32_t mul10(uint32_t n);

//TODO find a way to make them const, forcing prevention of thread safety issues, constexpr??
#ifdef __AVX512F__
  static __m512i _512_vec_A_minus_1;
  static __m512i _512_vec_case_range;
  static __m512i _512_add_mask;
#endif

#ifdef __AVX2__
  static __m256i _256_vec_A_minus_1;
  static __m256i _256_vec_case_range;
  static __m256i _256_add_mask;
#endif

#ifdef __SSE2__
  static __m128i _128_vec_A_minus_1;
  static __m128i _128_vec_case_range;
  static __m128i _128_add_mask;
#endif

CONSTRUCTOR void http_deserializer_init(void)
{
#ifdef __AVX512F__
  _512_vec_A_minus_1 = _mm512_set1_epi8('A' - 1);
  _512_vec_case_range = _mm512_set1_epi8('Z' - 'A' + 1);
  _512_add_mask = _mm512_set1_epi8('a' - 'A');
#endif

#ifdef __AVX2__
  _256_vec_A_minus_1 = _mm256_set1_epi8('A' - 1);
  _256_vec_case_range = _mm256_set1_epi8('Z' - 'A' + 1);
  _256_add_mask = _mm256_set1_epi8('a' - 'A');
#endif

#ifdef __SSE2__
  _128_vec_A_minus_1 = _mm_set1_epi8('A' - 1);
  _128_vec_case_range = _mm_set1_epi8('Z' - 'A' + 1);
  _128_add_mask = _mm_set1_epi8('a' - 'A');
#endif
}

//TODO maybe buffer size is needed for safety. memchr instead of rawmemchr?
uint32_t http1_deserialize(char *restrict buffer, UNUSED const uint32_t buffer_size, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;

  buffer += deserialize_status_code(buffer, response);
  buffer += deserialize_reason_phrase(buffer, response);
  buffer += deserialize_headers(buffer, response);

  response->body = buffer;

  return buffer - buffer_start;
}

static uint16_t deserialize_status_code(const char *buffer, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;

  buffer = rawmemchr(buffer, ' ');
  const uint16_t status_code = atoui(buffer, &buffer);
  response->status_code = status_code;
  buffer++;

  return ((uint16_t)(status_code - 100) < 499) * (buffer - buffer_start);
}

static uint16_t deserialize_reason_phrase(char *buffer, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;

  buffer = rawmemchr(buffer, '\r');
  *buffer = '\0';

  response->reason_phrase = buffer_start;
  response->reason_phrase_len = buffer - buffer_start;

  buffer += STR_LEN("\r\n");

  return buffer - buffer_start;
}

static uint16_t deserialize_headers(char *restrict buffer, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;

  const uint16_t header_map_size = response->headers.size;
  http_header_map_t *const header_map = &response->headers;
  uint16_t headers_count = 0;

  while (LIKELY(*buffer != '\r'))
  {
    char *const key = buffer;
    buffer = rawmemchr(buffer, ':');
    const uint16_t key_len = buffer - key;
    *buffer++ = '\0';
    buffer += strspn(buffer, " \t");

    strtolower(key, key_len);

    const char *const value = buffer;
    buffer = rawmemchr(buffer, '\r');
    const uint16_t value_len = buffer - value;
    *buffer = '\0';
    buffer += STR_LEN("\r\n");

    const http_header_t header = {
      .key = key,
      .key_len = key_len,
      .value = value,
      .value_len = value_len
    };

    if (UNLIKELY(headers_count++ == header_map_size))
      return 0;

    header_map_set(header_map, &header);
  }

  buffer += STR_LEN("\r\n");
  response->headers_count = headers_count;

  return buffer - buffer_start;
}

static void header_map_set(http_header_map_t *const restrict map, const http_header_t *const restrict header)
{
  const uint16_t map_size = map->size;
  const uint16_t original_idx = (uint16_t)(XXH3_64bits(header->key, header->key_len) % map_size);

  uint16_t idx = original_idx;
  for (uint16_t i = 1; UNLIKELY(map->entries[idx].key != NULL); i++)
    idx = (original_idx + i * i) % map_size;

  map->entries[idx] = *header;
}

const char *header_map_get(const http_header_map_t *const restrict map, const char *const key, const uint16_t key_len)
{
  const uint16_t map_size = map->size;
  const uint16_t original_idx = (uint16_t)(XXH3_64bits(key, key_len) % map_size);

  uint16_t idx = original_idx;
  for (uint16_t i = 0; LIKELY(map->entries[idx].key != NULL); i++)
  {
    if (UNLIKELY(memcmp(map->entries[idx].key, key, key_len) == 0))
      return map->entries[idx].value;

    idx = (original_idx + i * i) % map_size;
  }

  return NULL;
}

static uint32_t atoui(const char *str, const char **const endptr)
{
  uint32_t result = 0;

  str += strspn(str, " \t\n\r\v\f");

  while (LIKELY(((uint8_t)(*str - '0') < 10)))
  {
    result = mul10(result) + (*str - '0');
    str++;
  }

  (void)(endptr && (*endptr = str));
  return result;
}

static inline uint32_t mul10(uint32_t n)
{
  return (n << 3) + (n << 1);
}

void strtolower(char *str, uint16_t len)
{
  uint8_t unaligned_bytes = align_forward(str);
  unaligned_bytes &= -(unaligned_bytes <= len);

  while (UNLIKELY(unaligned_bytes--))
  {
    const char c = *str;
    *str++ = c | (((uint8_t)(c - 'A') <= ('Z' - 'A')) << 5);
    len--;
  }

#ifdef __AVX512F__
  while (LIKELY(len >= 64))
  {
    __m512i chunk = _mm512_load_si512((__m512i *)str);

    const __m512i shifted = _mm512_xor_si512(chunk, _512_vec_A_minus_1);
    const __mmask64 cmp_mask = _mm512_cmple_epi8_mask(shifted, _512_vec_case_range);
    const __m512i add_mask = _mm512_maskz_mov_epi8(cmp_mask, _512_add_mask);

    chunk = _mm512_add_epi8(chunk, add_mask);
    _mm512_stream_si512((__m512i *)str, chunk);

    str += 64;
    len -= 64;
  }
#endif

#ifdef __AVX2__  
  while (LIKELY(len >= 32))
  {
    __m256i chunk = _mm256_load_si256((__m256i *)str);

    const __m256i shifted = _mm256_xor_si256(chunk, _256_vec_A_minus_1);
    const __m256i cmp_mask = _mm256_cmpgt_epi8(_256_vec_case_range, shifted);
    const __m256i add_mask = _mm256_and_si256(cmp_mask, _256_add_mask);

    chunk = _mm256_add_epi8(chunk, add_mask);

    _mm256_stream_si256((__m256i *)str, chunk);

    str += 32;
    len -= 32;
  }
#endif

#ifdef __SSE2__
  while (LIKELY(len >= 16))
  {
    __m128i chunk = _mm_load_si128((__m128i *)str);

    const __m128i shifted = _mm_xor_si128(chunk, _128_vec_A_minus_1);
    const __m128i cmp_mask = _mm_cmpgt_epi8(_128_vec_case_range, shifted);
    const __m128i add_mask = _mm_and_si128(cmp_mask, _128_add_mask);

    chunk = _mm_add_epi8(chunk, add_mask);
    _mm_stream_si128((__m128i *)str, chunk);

    str += 16;
    len -= 16;
  }
#endif

  constexpr uint64_t all_bytes = 0x0101010101010101ULL;

  while (LIKELY(len >= 8))
  {
    const uint64_t octets = *(uint64_t *)str;
    const uint64_t heptets = octets & (0x7F * all_bytes);
    const uint64_t is_gt_Z = heptets + (0x7F - 'Z') * all_bytes;
    const uint64_t is_ge_A = heptets + (0x80 - 'A') * all_bytes;
    const uint64_t is_ascii = ~octets & (0x80 * all_bytes);
    const uint64_t is_upper = is_ascii & (is_ge_A ^ is_gt_Z);

    *(uint64_t *)str = octets | (is_upper >> 2);

    str += 8;
    len -= 8;
  }

  while (LIKELY(len--))
  {
    const char c = *str;
    *str++ = c | (((uint8_t)(c - 'A') <= ('Z' - 'A')) << 5);
  }
}
