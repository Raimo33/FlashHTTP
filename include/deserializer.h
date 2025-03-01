/*================================================================================

File: deserializer.h                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-03-01 17:39:45                                                

================================================================================*/

#ifndef FLASHFIX_DESERIALIZER_H
# define FLASHFIX_DESERIALIZER_H

# include <stdint.h>

# include "structs.h"

uint32_t http1_deserialize(char *restrict buffer, const uint32_t buffer_size, http_response_t *const restrict response);

const char *header_map_get(const http_header_map_t *const restrict map, const char *const key, const uint16_t key_len);

#endif