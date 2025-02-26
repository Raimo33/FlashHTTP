/*================================================================================

File: serializer.h                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-02-26 21:40:21                                                

================================================================================*/

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include <stdint.h>

# include "structs.h"

uint16_t fh_serialize(char *restrict buffer, const http_request_t *restrict request);

#endif