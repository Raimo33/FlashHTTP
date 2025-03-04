/*================================================================================

File: test.c                                                                    
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-10 21:08:13                                                 
last edited: 2025-03-04 18:41:41                                                

================================================================================*/

#include <flashttp.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>

#define STR_LEN(x) (sizeof(x) - 1)
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

static bool compare_headers(const http_header_t *headers, const http_header_t *expected_headers, const uint16_t headers_count)
{
  for (uint16_t i = 0; i < headers_count; i++)
  {
    const bool values_match = memcmp(headers[i].value, expected_headers[i].value, headers[i].value_len) == 0;
    const bool keys_match = memcmp(headers[i].key, expected_headers[i].key, headers[i].key_len) == 0;

    if (!values_match || !keys_match)
      return false;
  }

  return true;
}

static char *all_tests(void);
static char *test_serialize_normal_message(void);
static char *test_serialize_no_headers(void);
static char *test_serialize_no_body(void);
static char *test_serialize_no_headers_no_body(void);

static char *test_serialize_write_normal_message(void);
static char *test_serialize_write_no_headers(void);
static char *test_serialize_write_no_body(void);
static char *test_serialize_write_no_headers_no_body(void);
static char *test_serialize_write_too_many_headers(void);

static char *test_deserialize_normal_message(void);
static char *test_deserialize_duplicate_headers(void);
static char *test_deserialize_no_headers(void);
static char *test_deserialize_no_body(void);
static char *test_deserialize_no_headers_no_body(void);
static char *test_deserialize_missing_status_code(void);
static char *test_deserialize_missing_reason_phrase(void);
static char *test_deserialize_too_many_headers(void);
static char *test_deserialize_missing_colon(void);
static char *test_deserialize_missing_header_key(void);
static char *test_deserialize_missing_header_value(void);
static char *test_deserialize_reason_phrase_too_long(void);
static char *test_deserialize_header_key_too_long(void);
static char *test_deserialize_header_value_too_long(void);

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
  mu_run_test(test_serialize_no_headers);
  mu_run_test(test_serialize_no_body);
  mu_run_test(test_serialize_no_headers_no_body);

  mu_run_test(test_serialize_write_normal_message);
  mu_run_test(test_serialize_write_no_headers);
  mu_run_test(test_serialize_write_no_body);
  mu_run_test(test_serialize_write_no_headers_no_body);
  mu_run_test(test_serialize_write_too_many_headers);

  mu_run_test(test_deserialize_normal_message);
  mu_run_test(test_deserialize_duplicate_headers);
  mu_run_test(test_deserialize_no_headers);
  mu_run_test(test_deserialize_no_body);
  mu_run_test(test_deserialize_no_headers_no_body);
  mu_run_test(test_deserialize_missing_status_code);
  mu_run_test(test_deserialize_missing_reason_phrase);
  mu_run_test(test_deserialize_too_many_headers);
  mu_run_test(test_deserialize_missing_colon);
  mu_run_test(test_deserialize_missing_header_key);
  mu_run_test(test_deserialize_missing_header_value);
  mu_run_test(test_deserialize_reason_phrase_too_long);
  mu_run_test(test_deserialize_header_key_too_long);
  mu_run_test(test_deserialize_header_value_too_long);
  //TODO tests with \r in body and \n

  return 0;
}

static char *test_serialize_normal_message(void)
{
  http_header_t headers[] = {
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
  char body[] = "This is the body of the request";
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .headers = headers,
    .headers_count = ARR_SIZE(headers),
    .body = body,
    .body_len = STR_LEN(body)
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
  const uint16_t expected_len = STR_LEN(expected_buffer);

  char buffer[sizeof(expected_buffer)] = {0};
  uint32_t len = http1_serialize(buffer, &request);

  mu_assert("error: serialize normal message: wrong length", len == expected_len);
  mu_assert("error: serialize normal message: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);

  return 0;
}

static char *test_serialize_no_headers(void)
{
  char body[] = "This is the body of the request";
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .body = body,
    .body_len = STR_LEN(body)
  };
  const char expected_buffer[] =
    "GET /example/path/resource HTTP/1.1\r\n"
    "\r\n"
    "This is the body of the request";
  const uint16_t expected_len = STR_LEN(expected_buffer);

  char buffer[sizeof(expected_buffer)] = {0};
  uint32_t len = http1_serialize(buffer, &request);

  mu_assert("error: serialize no headers: wrong length", len == expected_len);
  mu_assert("error: serialize no headers: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);

  return 0;
}

static char *test_serialize_no_body(void)
{
  http_header_t headers[] = {
    { .key = "Host", .value = "example.com", .key_len = 4, .value_len = 11 }
  };
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .headers = headers,
    .headers_count = ARR_SIZE(headers)
  };
  const char expected_buffer[] =
    "GET /example/path/resource HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "\r\n";
  const uint16_t expected_len = STR_LEN(expected_buffer);

  char buffer[sizeof(expected_buffer)] = {0};
  uint32_t len = http1_serialize(buffer, &request);

  mu_assert("error: serialize no body: wrong length", len == expected_len);
  mu_assert("error: serialize no body: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);

  return 0;
}

static char *test_serialize_no_headers_no_body(void)
{
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1
  };
  const char expected_buffer[] =
    "GET /example/path/resource HTTP/1.1\r\n"
    "\r\n";
  const uint16_t expected_len = STR_LEN(expected_buffer);

  char buffer[sizeof(expected_buffer)] = {0};
  uint32_t len = http1_serialize(buffer, &request);

  mu_assert("error: serialize no headers no body: wrong length", len == expected_len);
  mu_assert("error: serialize no headers no body: wrong buffer", memcmp(buffer, expected_buffer, len) == 0);

  return 0;
}

static char *test_serialize_write_normal_message(void)
{
  http_header_t headers[] = {
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
  char body[] = "This is the body of the request";
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .headers = headers,
    .headers_count = ARR_SIZE(headers),
    .body = body,
    .body_len = STR_LEN(body)
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
  const uint16_t expected_len = STR_LEN(expected_buffer);

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

static char *test_serialize_write_no_headers(void)
{
  char body[] = "This is the body of the request";
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .body = body,
    .body_len = STR_LEN(body)
  };
  const char expected_buffer[] =
    "GET /example/path/resource HTTP/1.1\r\n"
    "\r\n"
    "This is the body of the request";
  const uint16_t expected_len = STR_LEN(expected_buffer);

  int fds[2];
  if (pipe(fds) == -1)
    return strerror(errno);
  int32_t len = http1_serialize_write(fds[1], &request);

  mu_assert("error: serialize write no headers: wrong length", len == expected_len);
  mu_assert("error: serialize write no headers: wrong output", compare_file(fds[0], expected_buffer, expected_len));

  close(fds[0]);
  close(fds[1]);

  return 0;
}

static char *test_serialize_write_no_body(void)
{
  http_header_t headers[] = {
    { .key = "Host", .value = "example.com", .key_len = 4, .value_len = 11 }
  };
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .headers = headers,
    .headers_count = ARR_SIZE(headers)
  };
  const char expected_buffer[] =
    "GET /example/path/resource HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "\r\n";
  const uint16_t expected_len = STR_LEN(expected_buffer);

  int fds[2];
  if (pipe(fds) == -1)
    return strerror(errno);
  int32_t len = http1_serialize_write(fds[1], &request);

  mu_assert("error: serialize write no body: wrong length", len == expected_len);
  mu_assert("error: serialize write no body: wrong output", compare_file(fds[0], expected_buffer, expected_len));

  close(fds[0]);
  close(fds[1]);

  return 0;
}

static char *test_serialize_write_no_headers_no_body(void)
{
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1
  };
  const char expected_buffer[] =
    "GET /example/path/resource HTTP/1.1\r\n"
    "\r\n";
  const uint16_t expected_len = STR_LEN(expected_buffer);

  int fds[2];
  if (pipe(fds) == -1)
    return strerror(errno);
  int32_t len = http1_serialize_write(fds[1], &request);

  mu_assert("error: serialize write no headers no body: wrong length", len == expected_len);
  mu_assert("error: serialize write no headers no body: wrong output", compare_file(fds[0], expected_buffer, expected_len));

  close(fds[0]);
  close(fds[1]);

  return 0;
}

static char *test_serialize_write_too_many_headers(void)
{
  http_header_t *headers = calloc(IOV_MAX, sizeof(http_header_t));
  if (headers == NULL)
    return strerror(errno);
  
  for (uint16_t i = 0; i < IOV_MAX; i++)
  {
    headers[i] = (http_header_t) {
      .key = "Header", .value = "Value", .key_len = 6, .value_len = 5
    };
  }

  char body[] = "This is the body of the request";
  const http_request_t request = {
    .method = HTTP_GET,
    .path = "/example/path/resource",
    .path_len = 22,
    .version = HTTP_1_1,
    .headers = headers,
    .headers_count = IOV_MAX,
    .body = body,
    .body_len = STR_LEN(body)
  };

  int fds[2];
  if (pipe(fds) == -1)
    return strerror(errno);
  int32_t len = http1_serialize_write(fds[1], &request);

  mu_assert("error: serialize write too many headers: should fail", len == -1);

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
    { .key = "Content-Type",    .value = "text/html; charset=UTF-8",      .key_len = 12,  .value_len = 24 },
    { .key = "Content-Length",  .value = "1234",                          .key_len = 14,  .value_len = 4 },
    { .key = "Connection",      .value = "keep-alive",                    .key_len = 10,  .value_len = 10 },
    { .key = "Server",          .value = "Apache/2.4.41 (Unix)",          .key_len = 6,   .value_len = 20 },
    { .key = "Cache-Control",   .value = "max-age=3600",                  .key_len = 13,  .value_len = 12 },
    { .key = "ETag",            .value = "\"abc123\"",                    .key_len = 4,   .value_len = 8 },
    { .key = "Date",            .value = "Mon, 01 Jan 2023 12:00:00 GMT", .key_len = 4,   .value_len = 29 }
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
  const uint32_t expected_len = STR_LEN(buffer) - STR_LEN(expected_body);

  http_header_t headers[ARR_SIZE(expected_headers)] = {0};
  http_response_t response = { .headers = headers, .headers_count = ARR_SIZE(headers) };
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize normal message: wrong length", len == expected_len);
  mu_assert("error: deserialize normal message: wrong status code", response.status_code == expected_status_code);
  mu_assert("error: deserialize normal message: wrong reason phrase", memcmp(response.reason_phrase, expected_reason_phrase, 2) == 0);
  mu_assert("error: deserialize normal message: wrong headers count", response.headers_count == 7);
  mu_assert("error: deserialize normal message: wrong headers", compare_headers(response.headers, expected_headers, response.headers_count));
  mu_assert("error: deserialize normal message: wrong body", memcmp(response.body, expected_body, sizeof(expected_body)) == 0);

  return 0;
}

static char *test_deserialize_duplicate_headers(void)
{
  char buffer[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: 1234\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 5678\r\n"
    "\r\n"
    "This is the body of the response";
  const uint16_t expected_status_code = 200;
  const char expected_reason_phrase[] = "OK";
  const http_header_t expected_headers[] = {
    { .key = "Content-Type",   .value = "text/html; charset=UTF-8", .key_len = 12, .value_len = 24 },
    { .key = "Content-Length", .value = "1234",                     .key_len = 14, .value_len = 4 },
    { .key = "Content-Type",   .value = "text/plain",               .key_len = 12, .value_len = 10 },
    { .key = "Content-Length", .value = "5678",                     .key_len = 14, .value_len = 4 }
  };
  const char expected_body[] = "This is the body of the response";
  const uint32_t expected_len = STR_LEN(buffer) - STR_LEN(expected_body);

  http_header_t headers[ARR_SIZE(expected_headers)] = {0};
  http_response_t response = { .headers = headers, .headers_count = ARR_SIZE(headers) };
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize duplicate headers: wrong length", len == expected_len);
  mu_assert("error: deserialize duplicate headers: wrong status code", response.status_code == expected_status_code);
  mu_assert("error: deserialize duplicate headers: wrong reason phrase", memcmp(response.reason_phrase, expected_reason_phrase, 2) == 0);
  mu_assert("error: deserialize duplicate headers: wrong headers count", response.headers_count == ARR_SIZE(expected_headers));
  mu_assert("error: deserialize duplicate headers: wrong headers", compare_headers(response.headers, expected_headers, response.headers_count));
  mu_assert("error: deserialize duplicate headers: wrong body", memcmp(response.body, expected_body, sizeof(expected_body)) == 0);

  return 0;
}

static char *test_deserialize_no_headers(void)
{
  char buffer[] =
    "HTTP/1.1 200 OK\r\n"
    "\r\n"
    "This is the body of the response";
  const uint16_t expected_status_code = 200;
  const char expected_reason_phrase[] = "OK";
  const char expected_body[] = "This is the body of the response";
  const uint32_t expected_len = STR_LEN(buffer) - STR_LEN(expected_body);

  http_response_t response = {0};
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize no headers: wrong length", len == expected_len);
  mu_assert("error: deserialize no headers: wrong status code", response.status_code == expected_status_code);
  mu_assert("error: deserialize no headers: wrong reason phrase", memcmp(response.reason_phrase, expected_reason_phrase, 2) == 0);
  mu_assert("error: deserialize no headers: wrong headers count", response.headers_count == 0);
  mu_assert("error: deserialize no headers: wrong body", memcmp(response.body, expected_body, sizeof(expected_body)) == 0);

  return 0;
}

static char *test_deserialize_no_body(void)
{
  char buffer[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "Content-Length: 1234\r\n"
    "\r\n";
  const uint16_t expected_status_code = 200;
  const char expected_reason_phrase[] = "OK";
  const http_header_t expected_headers[] = {
    { .key = "Content-Type",   .value = "text/html; charset=UTF-8", .key_len = 12, .value_len = 24 },
    { .key = "Content-Length", .value = "1234",                     .key_len = 14, .value_len = 4 }
  };
  const uint32_t expected_len = STR_LEN(buffer);

  http_header_t headers[ARR_SIZE(expected_headers)] = {0};
  http_response_t response = { .headers = headers, .headers_count = ARR_SIZE(headers) };
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize no body: wrong length", len == expected_len);
  mu_assert("error: deserialize no body: wrong status code", response.status_code == expected_status_code);
  mu_assert("error: deserialize no body: wrong reason phrase", memcmp(response.reason_phrase, expected_reason_phrase, 2) == 0);
  mu_assert("error: deserialize no body: wrong headers count", response.headers_count == ARR_SIZE(expected_headers));
  mu_assert("error: deserialize no body: wrong headers", compare_headers(response.headers, expected_headers, response.headers_count));
  mu_assert("error: deserialize no body: wrong body", response.body == NULL || response.body[0] == '\0');

  return 0;
}

static char *test_deserialize_no_headers_no_body(void)
{
  char buffer[] =
    "HTTP/1.1 200 OK\r\n"
    "\r\n";
  const uint16_t expected_status_code = 200;
  const char expected_reason_phrase[] = "OK";
  const uint32_t expected_len = STR_LEN(buffer);

  http_response_t response = {0};
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize no headers no body: wrong length", len == expected_len);
  mu_assert("error: deserialize no headers no body: wrong status code", response.status_code == expected_status_code);
  mu_assert("error: deserialize no headers no body: wrong reason phrase", memcmp(response.reason_phrase, expected_reason_phrase, 2) == 0);
  mu_assert("error: deserialize no headers no body: wrong headers count", response.headers_count == 0);
  mu_assert("error: deserialize no headers no body: wrong body", response.body == NULL || response.body[0] == '\0');

  return 0;
}

static char *test_deserialize_missing_status_code(void)
{
  char buffer[] =
    "HTTP/1.1 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "\r\n"
    "This is the body of the response";
  http_response_t response = {0};
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize missing status code: should fail", len == 0);

  return 0;
}

static char *test_deserialize_missing_reason_phrase(void)
{
  char buffer[] =
    "HTTP/1.1 200\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n"
    "\r\n"
    "This is the body of the response";
  http_header_t headers[1] = {0};
  http_response_t response = { .headers = headers, .headers_count = 1 };
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize missing reason phrase: should fail", len == 0);

  return 0;
}

static char *test_deserialize_too_many_headers(void)
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
    "Content-Type: text/plain\r\n"
    "Content-Length: 5678\r\n"
    "\r\n"
    "This is the body of the response";
  http_header_t headers[8] = {0};
  http_response_t response = { .headers = headers, .headers_count = 8 };
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize too many headers 1: should fail", len == 0);

  return 0;
}

static char *test_deserialize_missing_colon(void)
{
  char buffer[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type text/html; charset=UTF-8\r\n"
    "Content-Length 1234\r\n"
    "\r\n"
    "This is the body of the response";
  http_header_t headers[2] = {0};
  http_response_t response = { .headers = headers, .headers_count = 2 };
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize missing colon: should fail", len == 0);

  return 0;
}

static char *test_deserialize_missing_header_key(void)
{
  char buffer[] =
    "HTTP/1.1 200 OK\r\n"
    ": text/html; charset=UTF-8\r\n"
    "Content-Length: 1234\r\n"
    "\r\n"
    "This is the body of the response";
  http_response_t response = {0};
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize missing header key: should fail", len == 0);

  return 0;
}

static char *test_deserialize_missing_header_value(void)
{
  char buffer[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type:\r\n"
    "Content-Length: 1234\r\n"
    "\r\n"
    "This is the body of the response";
  http_response_t response = {0};
  const uint32_t len = http1_deserialize(buffer, sizeof(buffer), &response);

  mu_assert("error: deserialize missing header value: should fail", len == 0);

  return 0;
}

static char *test_deserialize_reason_phrase_too_long(void)
{
  char *buffer = calloc(UINT16_MAX * 2, sizeof(char));
  if (buffer == NULL)
    return strerror(errno);

  sprintf(buffer, "HTTP/1.1 200 ");
  for (uint16_t i = 0; i < UINT16_MAX; i++)
    strcat(buffer, "A");
  strcat(buffer, "\r\n\r\n");

  http_response_t response = {0};
  const uint32_t len = http1_deserialize(buffer, UINT16_MAX * 2, &response);

  free(buffer);

  mu_assert("error: deserialize reason phrase too long: should fail", len == 0);

  return 0;
}

static char *test_deserialize_header_key_too_long(void)
{
  char *buffer = calloc(UINT16_MAX * 2, sizeof(char));
  if (buffer == NULL)
    return strerror(errno);

  sprintf(buffer, "HTTP/1.1 200 OK\r\n");
  for (uint32_t i = 0; i < UINT16_MAX + 1; i++)
    strcat(buffer, "A");
  strcat(buffer, ": value\r\n\r\n");

  http_header_t headers[1] = {0};
  http_response_t response = { .headers = headers, .headers_count = 1 };
  const uint32_t len = http1_deserialize(buffer, UINT16_MAX * 2, &response);

  free(buffer);

  mu_assert("error: deserialize header key too long: should fail", len == 0);

  return 0;
}

static char *test_deserialize_header_value_too_long(void)
{
  char *buffer = calloc(UINT16_MAX * 2, sizeof(char));
  if (buffer == NULL)
    return strerror(errno);

  sprintf(buffer, "HTTP/1.1 200 OK\r\nHeader: ");
  for (uint32_t i = 0; i < UINT16_MAX + 1; i++)
    strcat(buffer, "A");
  strcat(buffer, "\r\n\r\n");

  http_header_t headers[1] = {0};
  http_response_t response = { .headers = headers, .headers_count = 1 };
  const uint32_t len = http1_deserialize(buffer, UINT16_MAX * 2, &response);

  free(buffer);

  mu_assert("error: deserialize header value too long: should fail", len == 0);

  return 0;
}