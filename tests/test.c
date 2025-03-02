/*================================================================================

File: test.c                                                                    
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-10 21:08:13                                                 
last edited: 2025-03-02 14:17:30                                                

================================================================================*/

#include <flashttp.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define ARR_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; if (message) return message; } while (0)
#define static_assert _Static_assert

uint32_t tests_run = 0;

static bool compare_file(int fd, const char *expected_buffer, const uint32_t expected_len)
{
  const uint32_t BUFFER_SIZE = 4096;
  char buffer[BUFFER_SIZE];
  uint32_t total_read = 0;

  while (total_read < expected_len)
  {
    const uint32_t remaining = expected_len - total_read;
    const uint32_t chunk_size = (remaining > BUFFER_SIZE) ? BUFFER_SIZE : remaining;

    const int32_t bytes_read = read(fd, buffer, chunk_size);
    if (bytes_read <= 0)
      return false;

    if (memcmp(buffer, expected_buffer + total_read, bytes_read) != 0)
      return false;

    total_read += bytes_read;
  }
  
  return true;
}

static bool compare_response_headers(const http_header_map_t *response_headers, const http_header_t *expected_headers, const uint16_t headers_count)
{
  for (uint16_t i = 0; i < headers_count; i++)
  {
    const char *header = header_map_get(response_headers, expected_headers[i].key, expected_headers[i].key_len);
    if (header == NULL)
      return false;

    if (memcmp(header, expected_headers[i].value, expected_headers[i].value_len) != 0)
      return false;
  }

  return true;
}

static char *all_tests(void);
static char *test_serialize_normal_message(void);
static char *test_serialize_write_normal_message(void);
static char *test_deserialize_normal_message(void);

int main(void)
{
  char *result = all_tests();

  if (result != 0)
    printf("%s\n", result);
  else
    printf("all tests passed\n");

  printf("Tests run: %d\n", tests_run);

  return !!result;
}

static char *all_tests(void)
{
  mu_run_test(test_serialize_normal_message);

  mu_run_test(test_serialize_write_normal_message);

  mu_run_test(test_deserialize_normal_message);

  return 0;
}

static char *test_serialize_normal_message(void)
{
  const http_header_t headers[] = {
    { .key = "Host",                      .value = "example.com",                       .key_len = 4,  .value_len = 11 },
    { .key = "User-Agent",                .value = "Mozilla/5.0",                       .key_len = 10, .value_len = 11 },
    { .key = "Accept",                    .value = "text/html",                         .key_len = 6,  .value_len = 9 },
    { .key = "Accept-Language",           .value = "en-US",                             .key_len = 15, .value_len = 5 },
    { .key = "Accept-Encoding",           .value = "gzip, deflate, br",                 .key_len = 15, .value_len = 17 },
    { .key = "Connection",                .value = "keep-alive",                        .key_len = 10, .value_len = 10 },
    { .key = "Referer",                   .value = "https://example.com/previous-page", .key_len = 7,  .value_len = 33 },
    { .key = "Upgrade-Insecure-Requests", .value = "1",                                 .key_len = 25, .value_len = 1 },
    { .key = "Sec-Fetch-Dest",            .value = "document",                          .key_len = 14, .value_len = 8 },
    { .key = "Sec-Fetch-Mode",            .value = "navigate",                          .key_len = 14, .value_len = 8 },
    { .key = "Sec-Fetch-Site",            .value = "same-origin",                       .key_len = 14, .value_len = 11 },
    { .key = "Sec-Fetch-User",            .value = "?1",                                .key_len = 14, .value_len = 2 }
  };
  const char body[] = "This is the body of the request";
  const http_request_t request = {
    .method = GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .headers = headers,
    .headers_count = 12,
    .body = body,
    .body_len = strlen(body)
  };
  const char expected_buffer[] =
    "GET /example/path/resource HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "User-Agent: Mozilla/5.0\r\n"
    "Accept: text/html\r\n"
    "Accept-Language: en-US\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Connection: keep-alive\r\n"
    "Referer: https://example.com/previous-page\r\n"
    "Upgrade-Insecure-Requests: 1\r\n"
    "Sec-Fetch-Dest: document\r\n"
    "Sec-Fetch-Mode: navigate\r\n"
    "Sec-Fetch-Site: same-origin\r\n"
    "Sec-Fetch-User: ?1\r\n"
    "\r\n"
    "This is the body of the request";
  const uint16_t expected_len = strlen(expected_buffer);

  char buffer[sizeof(expected_buffer)] = {0};
  uint32_t len = http1_serialize(buffer, &request);

  mu_assert("error: serialize normal message: wrong length", len == expected_len);
  mu_assert("error: serialize normal message: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);

  return 0;
}

static char *test_serialize_write_normal_message(void)
{
  const http_header_t headers[] = {
    { .key = "Host",                      .value = "example.com",                       .key_len = 4,  .value_len = 11 },
    { .key = "User-Agent",                .value = "Mozilla/5.0",                       .key_len = 10, .value_len = 11 },
    { .key = "Accept",                    .value = "text/html",                         .key_len = 6,  .value_len = 9 },
    { .key = "Accept-Language",           .value = "en-US",                             .key_len = 15, .value_len = 5 },
    { .key = "Accept-Encoding",           .value = "gzip, deflate, br",                 .key_len = 15, .value_len = 17 },
    { .key = "Connection",                .value = "keep-alive",                        .key_len = 10, .value_len = 10 },
    { .key = "Referer",                   .value = "https://example.com/previous-page", .key_len = 7,  .value_len = 33 },
    { .key = "Upgrade-Insecure-Requests", .value = "1",                                 .key_len = 25, .value_len = 1 },
    { .key = "Sec-Fetch-Dest",            .value = "document",                          .key_len = 14, .value_len = 8 },
    { .key = "Sec-Fetch-Mode",            .value = "navigate",                          .key_len = 14, .value_len = 8 },
    { .key = "Sec-Fetch-Site",            .value = "same-origin",                       .key_len = 14, .value_len = 11 },
    { .key = "Sec-Fetch-User",            .value = "?1",                                .key_len = 14, .value_len = 2 }
  };
  const char body[] = "This is the body of the request";
  const http_request_t request = {
    .method = GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .headers = headers,
    .headers_count = 12,
    .body = body,
    .body_len = strlen(body)
  };
  const char expected_buffer[] =
    "GET /example/path/resource HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "User-Agent: Mozilla/5.0\r\n"
    "Accept: text/html\r\n"
    "Accept-Language: en-US\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Connection: keep-alive\r\n"
    "Referer: https://example.com/previous-page\r\n"
    "Upgrade-Insecure-Requests: 1\r\n"
    "Sec-Fetch-Dest: document\r\n"
    "Sec-Fetch-Mode: navigate\r\n"
    "Sec-Fetch-Site: same-origin\r\n"
    "Sec-Fetch-User: ?1\r\n"
    "\r\n"
    "This is the body of the request";
  const uint16_t expected_len = strlen(expected_buffer);

  int fds[2];
  if (pipe(fds) == -1)
    return strerror(errno);
  int32_t len = http1_serialize_write(fds[1], &request);

  mu_assert("error: serialize write normal message: wrong length", len == expected_len);
  mu_assert("error: serialize write normal message: wrong output", compare_file(fds[0], expected_buffer, expected_len));

  close(fds[0]);
  close(fds[1]);

  return 0;
}

static char *test_deserialize_normal_message(void)
{
  char buffer[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: 1234\r\n"
    "Connection: keep-alive\r\n"
    "Server: Apache/2.4.41 (Unix)\r\n"
    "Cache-Control: max-age=3600\r\n"
    "ETag: \"abc123\"\r\n"
    "Date: Mon, 01 Jan 2023 12:00:00 GMT\r\n"
    "\r\n"
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "<title>Example Page</title>\n"
    "</head>\n"
    "<body>\n"
    "<h1>This is the body of the response</h1>\n"
    "</body>\n"
    "</html>";
  const uint16_t expected_status_code = 200;
  const char expected_reason_phrase[] = "OK";
  const http_header_t expected_headers[] = {
    { .key = "content-type",    .value = "text/html; charset=UTF-8",      .key_len = 12,  .value_len = 24 },
    { .key = "content-length",  .value = "1234",                          .key_len = 14,  .value_len = 4 },
    { .key = "connection",      .value = "keep-alive",                    .key_len = 10,  .value_len = 10 },
    { .key = "server",          .value = "Apache/2.4.41 (Unix)",          .key_len = 6,   .value_len = 20 },
    { .key = "cache-control",   .value = "max-age=3600",                  .key_len = 13,  .value_len = 12 },
    { .key = "etag",            .value = "\"abc123\"",                    .key_len = 4,   .value_len = 8 },
    { .key = "date",            .value = "Mon, 01 Jan 2023 12:00:00 GMT", .key_len = 4,   .value_len = 29 }
  };
  const char expected_body[] =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "<title>Example Page</title>\n"
    "</head>\n"
    "<body>\n"
    "<h1>This is the body of the response</h1>\n"
    "</body>\n"
    "</html>";
  const uint32_t expected_len = strlen(buffer) - strlen(expected_body);

  http_header_t headers[HEADER_MAP_CAPACITY(7)] = {0};
  http_response_t response = {
    .headers = {
      .entries = headers,
      .size = ARR_SIZE(headers)
    }
  };
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize normal message: wrong length", len == expected_len);
  mu_assert("error: deserialize normal message: wrong status code", response.status_code == expected_status_code);
  mu_assert("error: deserialize normal message: wrong reason phrase", memcmp(response.reason_phrase, expected_reason_phrase, 2) == 0);
  mu_assert("error: deserialize normal message: wrong headers count", response.headers_count == 7);
  mu_assert("error: deserialize normal message: wrong headers", compare_response_headers(&response.headers, expected_headers, 7));
  mu_assert("error: deserialize normal message: wrong body", memcmp(response.body, expected_body, sizeof(expected_body)) == 0);

  return 0;
}