# Deserialization

The following function prototypes can be found in the `deserialization.h` header file.

```c
#include <flashttp/deserialization.h>
```

These functions **only verify the structural integrity** of messages in terms of format. The body is served raw, without any decoding or parsing. It is up to the user to interpret the headers and eventually decode the body. Duplicate headers are not concatenated, but stored as separate fields.

## http1_deserialize

```c
uint32_t http1_deserialize(char *restrict buffer, const uint32_t buffer_size, http_response_t *const restrict response);
```

### Description
deserializes a http1 response in-place by replacing delimiters with `'\0'` and storing the pointers to the fields in the `response` struct.

### Parameters
  - `buffer` - the buffer which contains the full serialized response
  - `buffer_size` - the size of the buffer in bytes
  - `response` - the response struct where to store the deserialized fields, with the following conditions:
    - `headers` already allocated with a number of fields that matches the expected number of headers
    - `headers_count` set to the number of allocated headers
    - everything else should be zeroed (`0`' or `NULL`)

### Returns
  - length of the deserialized message in bytes, minus the body
  - `0` in case of error (see [Errors](#errors))

### Undefined Behavior
  - `buffer` is `NULL`
  - `response` is `NULL`
  - `response->headers` is not allocated
  - `buffer_size` is different from the actual size of the buffer
  - `buffer` does not contain a full http response

### Errors
  - wrong or missing status code
  - missing reason phrase
  - too many headers
  - missing colon in header
  - missing header key
  - missing header value
  - more than UINT16_MAX headers in the response
  - reason phrase longer than UINT16_MAX
  - header key longer than UINT16_MAX
  - header value longer than UINT16_MAX