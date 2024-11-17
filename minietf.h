#ifndef _MINIETF_H_
#define _MINIETF_H_
#include <stdint.h>

enum {
  etf_nil,
  etf_int, // stored in val
  etf_list, // stored in len/list
  etf_binary // stored in blob/list
};

enum {
  etf_read_okay = 100,
  etf_read_eof,
  etf_read_error,
  
  etf_read_fd_negative,
  etf_read_null_struct,

  etf_read_bad_magic,
  etf_read_not_tuple,
  etf_read_bad_tuple_len,
  etf_read_bad_ext,
  etf_read_bad_val,

  etf_read_bad_len,
  etf_read_unknown,
};

#define KEY_STORE (33)
#define KEY_MAX (KEY_STORE-1)

struct etf_tuple {
  uint8_t type;
  int32_t val;
  char key[KEY_STORE];
  uint8_t count;
  uint8_t *blob;
  int32_t *list;
  int len;
  int error;
};

int etf_parse(int fd, struct etf_tuple *tuple);
int etf_write(int ft, struct etf_tuple *tuple);

#endif