/*================================================================================

File: deserializer.c                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-27 19:39:29                                                

================================================================================*/

#include "common.h"
#include "deserializer.h"

static uint32_t atoui(const char *str, const char **endptr);
static inline uint32_t mul10(uint32_t n);

#ifdef __AVX512F__

#endif

#ifdef __AVX2__

#endif

#ifdef __SSE2__

#endif

CONSTRUCTOR void http_deserializer_init(void)
{
#ifdef __AVX512F__

#endif

#ifdef __AVX2__

#endif

#ifdef __SSE2__

#endif
}

uint16_t http1_deserialize(const char *buffer, const uint16_t buffer_size, http_message_t *message)
{
  //TODO
}

bool http1_is_complete(const char *buffer, const uint16_t len)
{
  //TODO
}

static uint32_t atoui(const char *str, const char **endptr)
{
  uint32_t result = 0;
  
  while (UNLIKELY(*str == ' '))
    str++;

  while (LIKELY(*str >= '0' && *str <= '9'))
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


