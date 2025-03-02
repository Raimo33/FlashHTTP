# Examples

## Serialization

```c
#include <flashttp/serialization.h>

#define STR_LEN(s)  (sizeof(s) - 1)
#define ARR_SIZE(a) (sizeof(a) / sizeof(a[0]))

/*...*/

const http_header_t headers[] = {
    { .key = "Host",                      .value = "example.com",                       STR_LEN("Host"), STR_LEN("example.com") },
    { .key = "User-Agent",                .value = "Mozilla/5.0",                       STR_LEN("User-Agent"), STR_LEN("Mozilla/5.0") },
    { .key = "Accept",                    .value = "text/html",                         STR_LEN("Accept"), STR_LEN("text/html") },
    { .key = "Accept-Language",           .value = "en-US",                             STR_LEN("Accept-Language"), STR_LEN("en-US") },
    { .key = "Accept-Encoding",           .value = "gzip, deflate, br",                 STR_LEN("Accept-Encoding"), STR_LEN("gzip, deflate, br") },
    { .key = "DNT",                       .value = "1",                                STR_LEN("DNT"), STR_LEN("1") },
    { .key = "Connection",                .value = "keep-alive",                        STR_LEN("Connection"), STR_LEN("keep-alive") },
    { .key = "Referer",                   .value = "https://example.com/previous-page", STR_LEN("Referer"), STR_LEN("https://example.com/previous-page") },
    { .key = "Upgrade-Insecure-Requests", .value = "1",                                 STR_LEN("Upgrade-Insecure-Requests"), STR_LEN("1") },
    { .key = "Sec-Fetch-Dest",            .value = "document",                          STR_LEN("Sec-Fetch-Dest"), STR_LEN("document") },
    { .key = "Sec-Fetch-Mode",            .value = "navigate",                          STR_LEN("Sec-Fetch-Mode"), STR_LEN("navigate") },
    { .key = "Sec-Fetch-Site",            .value = "same-origin",                       STR_LEN("Sec-Fetch-Site"), STR_LEN("same-origin") },
    { .key = "Sec-Fetch-User",            .value = "?1",                                STR_LEN("Sec-Fetch-User"), STR_LEN("?1") }
  };
const char body[] = "This is the body of the request";
const http_request_t request = {
  .method = GET,
  .path = "/example/path/resource",
  .path_len = STR_LEN("/example/path/resource"),
  .version = HTTP_1_1,
  .headers = headers,
  .headers_count = ARR_SIZE(headers),
  .body = body,
  .body_len = STR_LEN(body)
};

{
  char serialized_request[4096];
  const uint16_t request_len = http1_serialize(buffer, &request);
  write(sockfd, raw_serialized_data, data_len);
}

{
  http1_serialize_write(sockfd, &request);
}

/*...*/

```

## Deserialization

```c
#include <flashttp/deserialization.h>

/*...*/

//TODO add example

/*...*/
```