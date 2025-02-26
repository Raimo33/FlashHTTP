/*================================================================================

File: serializer.c                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-26 21:40:21                                                

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

uint16_t fh_serialize(char *restrict buffer, const http_request_t request)
{
 //TODO
}

