/*================================================================================

File: deserializer.h                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-28 19:02:45                                                

================================================================================*/

#ifndef FLASHFIX_DESERIALIZER_H
# define FLASHFIX_DESERIALIZER_H

# include <stdint.h>

# include "structs.h"

uint16_t http1_deserialize(char *restrict buffer, const uint16_t buffer_size, http_response_t *const restrict response);
bool http1_is_complete(const char *const restrict buffer, const uint16_t len);

const char *header_map_get(const http_header_map_t *const restrict map, const char *const key, const uint8_t key_len);

#endif