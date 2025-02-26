/*================================================================================

File: deserializer.h                                                            
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-26 21:40:21                                                

================================================================================*/

#ifndef FLASHFIX_DESERIALIZER_H
# define FLASHFIX_DESERIALIZER_H

# include <stdint.h>

# include "structs.h"

uint16_t fh_deserialize(const char *restrict buffer, const uint16_t buffer_size, http_response_t *restrict response);
bool fh_is_complete(const char *buffer, const uint16_t len);

#endif