/*================================================================================

File: structs.h                                                                 
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-13 13:38:07                                                 
last edited: 2025-03-04 12:17:51                                                

================================================================================*/

#ifndef STRUCTS_H
# define STRUCTS_H

# include <stdint.h>

typedef enum: uint8_t {
  HTTP_GET,
  HTTP_HEAD,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_OPTIONS,
  HTTP_TRACE,
  HTTP_PATCH,
  HTTP_CONNECT
} http_method_t;

typedef enum: uint8_t {
  HTTP_1_0,
  HTTP_1_1,
  HTTP_2_0,
  HTTP_3_0
} http_version_t;

//TODO alignment??

typedef struct
{
  char *key;
  char *value;
  uint16_t key_len;
  uint16_t value_len;
} http_header_t;

typedef struct
{
  http_method_t method;
  char *path;
  uint16_t path_len;
  http_version_t version;
  http_header_t *headers;
  uint16_t headers_count;
  char *body;
  uint32_t body_len;
} http_request_t;

typedef struct
{
  uint16_t status_code;
  char *reason_phrase;
  uint16_t reason_phrase_len;
  http_header_t *headers;
  uint16_t headers_count;
  char *body;
} http_response_t;

#endif