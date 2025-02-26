/*================================================================================

File: structs.h                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-13 13:38:07                                                 
last edited: 2025-02-26 21:40:21                                                

================================================================================*/

#ifndef STRUCTS_H
# define STRUCTS_H

# include <stdint.h>

# ifndef HTTP_MAX_HEADERS
#  define HTTP_MAX_HEADERS 64
# endif

# define HEADER_MAP_DILUTION_FACTOR 5

typedef enum: uint8_t {GET, POST, PUT, DELETE} http_method_t;
typedef enum: uint8_t {HTTP_1_0, HTTP_1_1} http_version_t;

//TODO alignment??
//TODO const?

typedef struct
{
  char *key;
  uint8_t key_len;
  char *value;
  uint8_t value_len;
} http_header_t;

typedef struct
{
  http_header_t headers[HTTP_MAX_HEADERS]; 
  uint8_t n_entries;
  uint16_t size;
} header_map_t;

typedef struct
{
  http_method_t method;
  char *path;
  uint8_t path_len;
  http_version_t version;
  http_header_t *headers;
  uint8_t n_headers;
  char *body;
  uint16_t body_len;
} http_request_t;

typedef struct
{
  http_header_t headers[HTTP_MAX_HEADERS];
  uint16_t headers_count;
  uint16_t status_code;
  char *body;
  uint16_t body_len;
} http_response_t;

#endif