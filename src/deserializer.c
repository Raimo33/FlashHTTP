/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-28 19:34:18                                                

================================================================================*/

#include "common.h"
#include "deserializer.h"

static uint16_t deserialize_status_code(const char *restrict buffer, uint16_t *const restrict status_code);
static uint16_t deserialize_reason_phrase(char *buffer, char **const reason_phrase);
static uint16_t deserialize_headers(char *restrict buffer, http_header_map_t *const restrict header_map, const uint16_t *restrict headers_count);
static void header_map_set(http_header_map_t *const restrict map, const http_header_t *const restrict header);
static uint32_t atoui(const char *str, const char **const endptr);
static inline uint32_t mul10(uint32_t n);

//TODO find a way to make them const, forcing prevention of thread safety issues, constexpr??
#ifdef __AVX512F__
  static __m512i _512_mask_A;
  static __m512i _512_mask_Z;
  static __m512i _512_add_mask;
#endif

#ifdef __AVX2__
  static __m256i _256_mask_A;
  static __m256i _256_mask_Z;
  static __m256i _256_add_mask;
#endif

#ifdef __SSE2__
  static __m128i _128_mask_A;
  static __m128i _128_mask_Z;
  static __m128i _128_add_mask;
#endif

CONSTRUCTOR void http_deserializer_init(void)
{
#ifdef __AVX512F__
  _512_mask_A = _mm512_set1_epi8('A');
  _512_mask_Z = _mm512_set1_epi8('Z');
  _512_add_mask = _mm512_set1_epi8('a' - 'A');
#endif

#ifdef __AVX2__
  _256_mask_A = _mm256_set1_epi8('A');
  _256_mask_Z = _mm256_set1_epi8('Z');
  _256_add_mask = _mm256_set1_epi8('a' - 'A');
#endif

#ifdef __SSE2__
  _128_mask_A = _mm_set1_epi8('A');
  _128_mask_Z = _mm_set1_epi8('Z');
  _128_add_mask = _mm_set1_epi8('a' - 'A');
#endif
}

uint16_t http1_deserialize(char *restrict buffer, const uint16_t buffer_size, http_response_t *const restrict response)
{
  const char *const buffer_start = buffer;

  buffer += deserialize_status_code(buffer, &response->status_code);
  buffer += deserialize_reason_phrase(buffer, &response->reason_phrase);
  buffer += deserialize_headers(buffer, response->headers, &response->headers_count);
  
  response->body = buffer;

  return buffer - buffer_start;
}

static uint16_t deserialize_status_code(const char *restrict buffer, uint16_t *const restrict status_code)
{
  const char *const buffer_start = buffer;

  buffer = rawmemchr(buffer, ' ');
  *status_code = atoui(buffer, &buffer);

  return ((uint16_t)(*status_code - 100) < 499) * (buffer - buffer_start);
}

static uint16_t deserialize_reason_phrase(char *buffer, char **const reason_phrase)
{
  const char *const buffer_start = buffer;

  buffer = rawmemchr(buffer, '\r');
  *reason_phrase = buffer;
  *buffer = '\0';
  buffer += STR_LEN("\r\n");

  return buffer - buffer_start;
}

static uint16_t deserialize_headers(char *restrict buffer, http_header_map_t *const restrict header_map, const uint16_t *restrict headers_count)
{
  const char *const buffer_start = buffer;

  while (LIKELY(*buffer != '\r'))
  {
    const char *const key = buffer;
    buffer = rawmemchr(buffer, ':');
    const uint16_t key_len = buffer - key;
    buffer++;
    buffer += strspn(buffer, " ");

    const char *const value = buffer;
    buffer = rawmemchr(buffer, '\r');
    const uint16_t value_len = buffer - value;
    buffer += STR_LEN("\r\n");

    const http_header_t header = {
      .key = key,
      .key_len = key_len,
      .value = value,
      .value_len = value_len
    };

    if (UNLIKELY(*headers_count++ == header_map->size))
      return 0;

    header_map_set(header_map, &header);
  }

  buffer += STR_LEN("\r\n");

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
  
  str += strspn(str, " ");

  while (LIKELY(*str >= '0' && *str <= '9')) //TODO bit trick
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
  const char *const aligned_str = align_forward(str);

  while (UNLIKELY(str < aligned_str && len))
  {
    const char c = *str;
    *str = c | (0x20 & (c - 'A') >> 8);

    len--;
    str++;
  }

#ifdef __AVX512F__
  while (LIKELY(len >= 64))
  {
    __m512i chunk = _mm512_load_si512((__m512i *)str);

    const __mmask64 is_upper = _mm512_movepi8_mask(_mm512_and_si512(
      _mm512_cmpge_epu8_mask(chunk, _512_mask_A),
      _mm512_cmple_epu8_mask(chunk, _512_mask_Z))
    );

    chunk = _mm512_mask_or_epi8(chunk, is_upper, chunk, _512_add_mask);

    _mm512_stream_si512((__m512i *)str, chunk);

    str += 64;
    len -= 64;
  }
#endif

#ifdef __AVX2__
  while (LIKELY(len >= 32))
  {
    __m256i chunk = _mm256_load_si256((__m256i *)str);

    const __m256i is_upper = _mm256_and_si256(
      _mm256_cmpgt_epi8(chunk, _256_mask_A),
      _mm256_cmplt_epi8(chunk, _256_mask_Z)
    );

    chunk = _mm256_or_si256(chunk, _mm256_and_si256(is_upper, _256_add_mask));

    _mm256_stream_si256((__m256i *)str, chunk);

    str += 32;
    len -= 32;
  }
#endif

#ifdef __SSE2__
  while (LIKELY(len >= 16))
  {
    __m128i chunk = _mm_load_si128((__m128i *)str);

    const __m128i is_upper = _mm_and_si128(
      _mm_cmpgt_epi8(chunk, _128_mask_A),
      _mm_cmplt_epi8(chunk, _128_mask_Z)
    );

    chunk = _mm_or_si128(chunk, _mm_and_si128(is_upper, _128_add_mask));

    _mm_stream_si128((__m128i *)str, chunk);

    str += 16;
    len -= 16;
  }
#endif

  constexpr uint64_t all_bytes = 0x0101010101010101;

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

  while (LIKELY(len))
  {
    const char c = *str;
    *str = c | (0x20 & (c - 'A') >> 8);

    len--;
    str++;
  }
}
