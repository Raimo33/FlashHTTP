# Serialization

The following function prototypes can be found in the `serialization.h` header file.

```c
#include <flashttp/serialization.h>
```

These functions **don't check the validity of messages**, they assume that the message struct is already correctly filled with the right values.

## http1_serialize

```c
uint32_t http1_serialize(char *restrict buffer, const http_request_t *restrict request)
```

### Description
serializes an HTTP/1.1 request into a buffer, adding separators where needed.

### Parameters
  - `buffer` - the buffer where to store the serialized request
  - `request` - the request struct containing the fields to serialize

### Returns
  - length of the serialized request in bytes

### Undefined Behavior
  - `buffer` is `NULL`
  - `request` is `NULL`
  - `request` doesn't fit in the buffer
  - `value_len`, `tag_len`, `path_len`, `body_len` are different from the actual lengths of the strings
  - `path` is NULL

## http1_serialize_write

```c
int32_t http1_serialize_write(const int fd, const http_request_t *restrict request)
```

### Description
serializes on the fly and writes an HTTP/1.1 request to a file descriptor.

### Parameters
  - `fd` - the file descriptor where to write the serialized request
  - `request` - the request struct containing the fields to serialize

### Returns
  - the result of the `writev` syscall
  - `-1` in case of error (see [Errors](#errors))

### Undefined Behavior
  - `fd` is not a valid file descriptor
  - `request` is `NULL`
  - `value_len`, `tag_len`, `path_len`, `body_len` are different from the actual lengths of the strings
  - `path` is NULL

### Errors
  - `writev` syscall error
  - the number of headers is bigger than what can be written in a single `writev` syscall, precisely if (`headers_count` * 4 > `IOV_MAX` - 9)

