/*================================================================================

File: serializer.h                                                              
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-11 12:37:26                                                 
last edited: 2025-03-01 17:39:45                                                

================================================================================*/

#ifndef FLASHFIX_SERIALIZER_H
# define FLASHFIX_SERIALIZER_H

# include <stdint.h>

# include "structs.h"

# define AVG_HEADER_COUNT 8
# define IOV_MAX __IOV_MAX

uint32_t http1_serialize(char *restrict buffer, const http_request_t *restrict request);
int32_t http1_serialize_write(const int fd, const http_request_t *restrict request);
//TODO support for http2 and http3
//TODO direct zero-copy write with writev

#endif