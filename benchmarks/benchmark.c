/*================================================================================

File: benchmark.c                                                               
Creator: Claudio Raimondi                                                       
Email: claudio.raimondi@pm.me                                                   

created at: 2025-02-14 17:53:51                                                 
last edited: 2025-03-05 21:11:45                                                

================================================================================*/

#include <flashhttp.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <immintrin.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define N_ITERATIONS 100'000
#define N_SAMPLES 500
#define MEAN_PATH_LEN 15
#define MAX_PATH_LEN 256
#define MEAN_HEADER_KEY_LEN 10
#define MAX_HEADER_KEY_LEN 32
#define MEAN_HEADER_VALUE_LEN 20
#define MAX_HEADER_VALUE_LEN 256
#define MEAN_BODY_LEN 256
#define MAX_BODY_LEN 4096
#define MEAN_HEADERS_COUNT 8
#define MAX_HEADERS_COUNT 64
#define MEAN_REASON_PHRASE_LEN 8
#define MAX_REASON_PHRASE_LEN 64
#define ALIGNMENT 64
#define BUFFER_SIZE (20 + MAX_PATH_LEN + MAX_REASON_PHRASE_LEN + 2 * (MAX_HEADER_KEY_LEN + MAX_HEADER_VALUE_LEN + 5) + MAX_BODY_LEN)
#define static_assert _Static_assert
#define STR_LEN(str) sizeof(str) - 1
#define ALIGNED(n) __attribute__((aligned(n)))

static void fill_request_structs(http_request_t *requests, uint16_t *path_lens, uint16_t *header_key_lens, uint16_t *header_value_lens, uint32_t *body_lens, uint16_t *headers_counts);
static void fill_response_buffers(char **buffers, uint16_t *path_lens, uint16_t *header_key_lens, uint16_t *header_value_lens, uint32_t *body_lens, uint16_t *headers_counts);
static void serialize(http_request_t *requests);
static void serialize_write(http_request_t *requests);
static void serialize_and_write(http_request_t *requests);
static void deserialize(char **buffers);
static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint32_t string_len);
static double gaussian_rand(const double mean, const double stddev);
static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max);
static uint32_t open_p(const char *pathname, const int32_t flags, const mode_t mode);
static void *calloc_p(const size_t n, const size_t size);
static void free_request_structs(http_request_t *requests);
static void free_response_buffers(char **buffers);

int32_t main(void)
{
  uint16_t path_lens[N_SAMPLES];
  uint16_t header_key_lens[N_SAMPLES];
  uint16_t header_value_lens[N_SAMPLES];
  uint32_t body_lens[N_SAMPLES];
  uint16_t headers_counts[N_SAMPLES];
  uint16_t reason_phrase_lens[N_SAMPLES];

  for (uint16_t i = 0; i < N_SAMPLES; i++)
  {
    path_lens[i] = gaussian_rand(MEAN_PATH_LEN, 5);
    header_key_lens[i] = gaussian_rand(MEAN_HEADER_KEY_LEN, 2);
    header_value_lens[i] = gaussian_rand(MEAN_HEADER_VALUE_LEN, 5);
    body_lens[i] = gaussian_rand(MEAN_BODY_LEN, 50);
    headers_counts[i] = gaussian_rand(MEAN_HEADERS_COUNT, 2);
    reason_phrase_lens[i] = gaussian_rand(MEAN_REASON_PHRASE_LEN, 2);

    path_lens[i] = clamp(path_lens[i], 0, MAX_PATH_LEN);
    header_key_lens[i] = clamp(header_key_lens[i], 1, MAX_HEADER_KEY_LEN);
    header_value_lens[i] = clamp(header_value_lens[i], 1, MAX_HEADER_VALUE_LEN);
    body_lens[i] = clamp(body_lens[i], 0, MAX_BODY_LEN);
    headers_counts[i] = clamp(headers_counts[i], 1, MAX_HEADERS_COUNT);
    reason_phrase_lens[i] = clamp(reason_phrase_lens[i], 1, MAX_REASON_PHRASE_LEN);
  }
  
  {
    http_request_t request_structs[N_SAMPLES];
    
    fill_request_structs(request_structs, path_lens, header_key_lens, header_value_lens, body_lens, headers_counts);
    
    serialize(request_structs);
    serialize_write(request_structs);
    serialize_and_write(request_structs);

    free_request_structs(request_structs);
  }

  {
    char *response_buffers[N_SAMPLES];

    fill_response_buffers(response_buffers, reason_phrase_lens, header_key_lens, header_value_lens, body_lens, headers_counts);

    deserialize(response_buffers);

    free_response_buffers(response_buffers);
  }
}

static void fill_request_structs(http_request_t *requests, uint16_t *path_lens, uint16_t *header_key_lens, uint16_t *header_value_lens, uint32_t *body_lens, uint16_t *headers_counts)
{
  constexpr char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  for (uint16_t i = 0; i < N_SAMPLES; i++)
  {
    requests[i].method = HTTP_GET;
    requests[i].path_len = path_lens[i];
    requests[i].path = generate_random_string(charset, STR_LEN(charset), path_lens[i]);
    requests[i].version = HTTP_1_1;
    requests[i].headers_count = headers_counts[i];
    requests[i].headers = calloc_p(headers_counts[i], sizeof(http_header_t));
    requests[i].body_len = body_lens[i];
    requests[i].body = generate_random_string(charset, STR_LEN(charset), body_lens[i]);

    for (uint16_t j = 0; j < headers_counts[i]; j++)
    {
      requests[i].headers[j].key_len = header_key_lens[i];
      requests[i].headers[j].key = generate_random_string(charset, STR_LEN(charset), header_key_lens[i]);
      requests[i].headers[j].value_len = header_value_lens[i];
      requests[i].headers[j].value = generate_random_string(charset, STR_LEN(charset), header_value_lens[i]);
    }
  }
}

static void fill_response_buffers(char **buffers, uint16_t *reason_phrase_lens, uint16_t *header_key_lens, uint16_t *header_value_lens, uint32_t *body_lens, uint16_t *headers_counts)
{
  constexpr char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  for (uint16_t i = 0; i < N_SAMPLES; i++)
  {
    buffers[i] = calloc_p(BUFFER_SIZE, sizeof(char));

    strcat(buffers[i], "HTTP/1.1 200 ");
    char *reason_phrase = generate_random_string(charset, STR_LEN(charset), reason_phrase_lens[i]);
    strcat(buffers[i], reason_phrase);
    strcat(buffers[i], "\r\n");
    free(reason_phrase);

    for (uint16_t j = 0; j < headers_counts[i]; j++)
    {
      char *key = generate_random_string(charset, STR_LEN(charset), header_key_lens[j]);
      char *value = generate_random_string(charset, STR_LEN(charset), header_value_lens[j]);

      strcat(buffers[i], key);
      strcat(buffers[i], ": ");
      strcat(buffers[i], value);
      strcat(buffers[i], "\r\n");

      free(key);
      free(value);
    }
    strcat(buffers[i], "\r\n");

    char *body = generate_random_string(charset, STR_LEN(charset), body_lens[i]);
    strcat(buffers[i], body);
    free(body);
  }
}

static void serialize(http_request_t *requests)
{
  uint64_t start, end;
  uint32_t aux;

  char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};
  uint64_t total_cycles = 0;

  printf("iterating http1_serialize() with %d samples %d times\n", N_SAMPLES, N_ITERATIONS);

  for (uint16_t i = 0; i < N_SAMPLES; i++)
  {
    http_request_t request = requests[i];

    start = __rdtscp(&aux);
    for (uint32_t j = 0; j < N_ITERATIONS; j++)
      http1_serialize(buffer, &request);
    end = __rdtscp(&aux);

    const uint64_t average_cycles = (end - start) / N_ITERATIONS;
    total_cycles += average_cycles;
  }

  printf("serialize(): avg CPU cycles: %lu\n", total_cycles / N_SAMPLES);
}

static void serialize_write(http_request_t *requests)
{  
  uint64_t start, end;
  uint32_t aux;
  uint64_t total_cycles = 0;

  const int32_t fd = open_p("/dev/null", O_WRONLY, 0);

  printf("iterating http1_serialize_write() with %d samples %d times\n", N_SAMPLES, N_ITERATIONS);

  for (uint16_t i = 0; i < N_SAMPLES; i++)
  {
    http_request_t request = requests[i];

    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      start = __rdtscp(&aux);
      http1_serialize_write(fd, &request);
      end = __rdtscp(&aux);

      total_cycles += (end - start);

      fsync(fd);
    }
  }

  printf("serialize_write(): avg CPU cycles: %lu\n", total_cycles / N_SAMPLES / N_ITERATIONS);

  close(fd);
}

static void serialize_and_write(http_request_t *requests)
{
  uint64_t start, end;
  uint32_t aux;
  uint64_t total_cycles = 0;

  char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT) = {0};
  const int32_t fd = open_p("/dev/null", O_WRONLY, 0);

  printf("iterating http1_serialize_and_write() with %d samples %d times\n", N_SAMPLES, N_ITERATIONS);

  for (uint16_t i = 0; i < N_SAMPLES; i++)
  {
    http_request_t request = requests[i];

    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      start = __rdtscp(&aux);
      const uint32_t len = http1_serialize(buffer, &request);
      write(fd, buffer, len);
      end = __rdtscp(&aux);

      total_cycles += (end - start);

      fsync(fd);
    }
  }

  printf("serialize_and_write(): avg CPU cycles: %lu\n", total_cycles / N_SAMPLES / N_ITERATIONS);

  close(fd);
}

static void deserialize(char **buffers)
{
  uint64_t start, end;
  uint32_t aux;
  uint64_t total_cycles = 0;

  http_header_t headers[MAX_HEADERS_COUNT] ALIGNED(ALIGNMENT);
  http_response_t response ALIGNED(ALIGNMENT) = { .headers = headers, .headers_count = MAX_HEADERS_COUNT };

  printf("iterating http1_deserialize() with %d samples %d times\n", N_SAMPLES, N_ITERATIONS);

  for (uint16_t i = 0; i < N_SAMPLES; i++)
  {
    char buffer[BUFFER_SIZE] ALIGNED(ALIGNMENT);

    for (uint32_t j = 0; j < N_ITERATIONS; j++)
    {
      memcpy(buffer, buffers[i], BUFFER_SIZE);

      start = __rdtscp(&aux);
      http1_deserialize(buffer, BUFFER_SIZE, &response);
      end = __rdtscp(&aux);

      total_cycles += (end - start);
    }
  }

  printf("deserialize(): avg CPU cycles: %lu\n", total_cycles / N_SAMPLES / N_ITERATIONS);
}

static char *generate_random_string(const char *charset, const uint8_t charset_len, const uint32_t string_len)
{
  char *str = calloc_p(string_len + 1, sizeof(char));

  for (uint16_t i = 0; i < string_len; i++)
    str[i] = charset[rand() % charset_len];
  str[string_len] = '\0';

  return str;
}

static double gaussian_rand(const double mean, const double stddev)
{
  const double u1 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
  const double u2 = ((double)rand() + 1) / ((double)RAND_MAX + 1);
  const double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
  return z0 * stddev + mean;
}

static inline uint16_t clamp(const uint16_t n, const uint16_t min, const uint16_t max)
{
  return n < min ? min : n > max ? max : n;
}

static uint32_t open_p(const char *pathname, const int32_t flags, const mode_t mode)
{
  const int32_t fd = open(pathname, flags, mode);
  if (fd == -1)
  {
    perror("open");
    exit(EXIT_FAILURE);
  }
  return fd;
}

static void *calloc_p(const size_t n, const size_t size)
{
  void *ptr = calloc(n, size);
  if (!ptr)
  {
    perror(strerror(errno));
    exit(EXIT_FAILURE);
  }
  return ptr;
}

static void free_request_structs(http_request_t *requests)
{
  for (uint16_t i = 0; i < N_SAMPLES; i++)
  {
    free(requests[i].path);
    free(requests[i].body);

    for (uint16_t j = 0; j < requests[i].headers_count; j++)
    {
      free(requests[i].headers[j].key);
      free(requests[i].headers[j].value);
    }

    free(requests[i].headers);
  }
}

static void free_response_buffers(char **buffers)
{
  for (uint16_t i = 0; i < N_SAMPLES; i++)
    free(buffers[i]);
}