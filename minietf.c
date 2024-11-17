#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "minietf.h"

#define FD_PRINTF_MAX (1024)

static char _fd_printf_buf[FD_PRINTF_MAX];

int fd_printf(int fd, const char * fmt, ...) {
  int n;
  va_list ap;
  va_start (ap, fmt);
  n = vsnprintf (_fd_printf_buf, FD_PRINTF_MAX, fmt, ap);
  va_end (ap);
  write (fd, _fd_printf_buf, n);
  return n;
}

#define ETF_LOG_ALL (0)
#define ETF_LOG_FATAL (1)
#define ETF_LOG_ERROR (2)
#define ETF_LOG_DEBUG (2)
#define ETF_LOG_INFO (4)

static int _etf_log_level = 1;

// it's only on or off for now... the defines above signal intent though...
#define LOG(...) if (_etf_log_level) fd_printf(STDERR_FILENO, __VA_ARGS__)

// Erlang terms we may parse or emit

#define ETF_MAGIC         (131)
#define ETF_SMALL_INTEGER (97)
#define ETF_INTEGER       (98)
#define ETF_SMALL_TUPLE   (104)
#define ETF_NIL           (106)
#define ETF_STRING        (107) // followed by two bytes of length (big-endian)
#define ETF_LIST          (108) // followed by four bytes of length
#define ETF_BINARY        (109) // followed by four bytes of length
#define ETF_ZLIB          (80)  // followed by four bytes of uncompressed length and a zlib stream...

/*

# ----

iex(2)> :erlang.term_to_binary([1,2,3], [:compressed])
<<131, 107, 0, 3, 1, 2, 3>>

iex(3)> a = :erlang.term_to_binary([1,2,3,5,6,7,8,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[:compressed])
<<131, 80, 0, 0, 0, 27, 120, 156, 203, 102, 144, 96, 100, 98, 102, 101, 99, 231,
  224, 100, 64, 3, 0, 16, 211, 0, 173>>

080 000 000 000 027 120 156 203 102 144 096 100 098 102 101 099 231 224 100 064 003 000 016 211 000 173
ZLB L3  L2  L1  L0  78  9C
    27 bytes------- ZMAGIC
    make a buffer to decompress to that's 27 bytes

iex(6)> b = :erlang.binary_to_term a
[1, 2, 3, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

iex(7)> length b
24

iex(9)> z = :erlang.binary_to_list a
[131, 80, 0, 0, 0, 27, 120, 156, 203, 102, 144, 96, 100, 98, 102, 101, 99, 231,
224, 100, 64, 3, 0, 16, 211, 0, 173]

iex(10)> length z
27

# -----
*/

#define CR "\n\r"

void etf_dump(struct etf_tuple *tuple) {
  if (!tuple) return;
  char *s = "";
  LOG("{");
  if (tuple->key) LOG("\"%s\"", tuple->key);
  switch (tuple->type) {
    case etf_nil:
      break;
    case etf_int:
      LOG(",%d", tuple->val);
      break;
    case etf_list:
      LOG(",[");
      for (int i=0; i<tuple->len; i++) {
        LOG("%s%d", s, tuple->list[i]);
        s = ",";
      }
      LOG("]");
      break;
    case etf_binary:
      LOG(",\"%s\"", tuple->blob);
      break;
    default:
      LOG("?"CR);
      break;
  }
  LOG("}"CR);
}

/*

port = Port.open({:spawn, "./crex1"}, [:binary])
data = :erlang.term_to_binary({})
Port.command(port, data)
Port.command(port, :erlang.term_to_binary({}))
Port.close()

{}
131 104 000

{"info"}
131 104 001 109 000 000 000 004 105 110 102 111

{"get-outs", []}
131 104 002 109 000 000 000 008 103 101 116 045 111 117 116 115 106
ETF STU LEN BIN L3  L2  L1  L0  g   e   t   -   o   u   t   s   NIL

{"ret-outs", [0, 1, 2]}
131 104 002 109 000 000 000 008 114 101 116 045 111 117 116 115 107 000 003 000 001 002
ETF STU LEN BIN L3  L2  L1  L0  r   e   t   -   o   u   t   s   STE L1  L0  1   2   3

{"get-outs", [1023]}
131 104 002 109 000 000 000 008 103 101 116 045 111 117 116 115 108 000 000 000 001 098 000 000 003 255 106
ETF STU LEN BIN L3  L2  L1  L0  r   e   t   -   o   u   t   s   LIE L3  L2  L1  L0  INE 1023            NIL

{"get-outs",[1, 1023]}
131 104 002 109 000 000 000 008 103 101 116 045 111 117 116 115 108 000 000 000 002 097 001 098 000 000 003 255 106
ETF STU LEN BIN L3  L2  L1  L0  r   e   t   -   o   u   t   s   LIE L3  L2  L1  L0  SIE 1   INE 1023            NIL

{"info","okay"}
131 104 002 109 000 000 000 004 105 110 102 111 109 000 000 000 004 111 107 097 121
ETF STU LEN BIN L3  L2  L1  L0  i   n   f   o   BIN L3  L2  L1  L0  o   k   a   y
*/

int readbn(int fd, void *m, size_t len) {
  int origlen = len;
  if (fd < 0) {
    // LOG("fd=%d"CR, fd);
    return -etf_read_fd_negative;
  }
  uint8_t *c = m;
  int n;
  while (len > 0) {
    n = read(fd, c++, len);
    if (n <= 0) return n;
    len -= n;
  }
  c = m;
  // LOG("readbn(%d,%p,%d) = %d"CR, fd, m, origlen, n);
  // if (n > 0) {
  //   for (int i=0; i<origlen; i++) LOG("%03d ", c[i]);
  //   LOG(CR);
  // }
  return origlen;
}

int readb4(int fd, uint32_t *four) {
  uint32_t n = 0;
  int r = readbn(fd, &n, sizeof(uint32_t));
  if (r <= 0) return r;
  n = be32toh(n);
  if (four) *four = n;
  return sizeof(uint32_t);
}

int readb2(int fd, uint16_t *two) {
  uint16_t n;
  int r = readbn(fd, &n, sizeof(uint16_t));
  if (r <= 0) return r;
  if (two) *two = be16toh(n);
  return sizeof(uint16_t);
}

int readb1(int fd, uint8_t *one) {
  uint8_t n = 0;
  int r = readbn(fd, &n, sizeof(uint8_t));
  if (r <= 0) return r;
  if (one) *one = n;
  return sizeof(uint8_t);
}

int readpast(int fd, size_t skip) {
  uint8_t ignore;
  for (int i=0; i<skip; i++) {
    int r = readb1(fd, &ignore);
    if (r <= 0) return r;
  }
  return skip;
}

int free_list_fail(struct etf_tuple *tuple, int r) {
  if (tuple && tuple->list) {
    free(tuple->list);
    tuple->list = NULL;
    tuple->len = 0;
  }
  return r;
}

enum {
  write_okay = 100,
} write_errors;

int etf_send(int fd, struct etf_tuple *tuple) {
  return write_okay;
}

/*
  goal to parse
  {}

  {"outs"}
  {"open", number}
  {"start"}
  {"stop"}
  {"close"}
  {"wire", "xxx"}
  {"put", []}
  {"put", [0,1,...]}
  {"put", [-32000,32000,...]}
  {"get"}


  {"key"}
  {"key", number}
  {"key", number, number}
  {"key", number, []}
  {"key", number, [0,1,2]}
  {"key", number, [-1,1024]}

  examples
  {"capture"}
  {"capture", devid}
  {"buffer", size}
  {"record", devid, bufid}
  {"play", devid, bufid}
  {"store", bufid, [0,1,2]}
  {"store", bufid, [-1000,1000]}
*/

int etf_write(int ft, struct etf_tuple *tuple) {
  return 0;
}

int etf_parse(int fd, struct etf_tuple *tuple) {
  LOG("etf_parse"CR);
  if (fd < 0) return -etf_read_fd_negative;
  if (!tuple) return -etf_read_null_struct;
 
  tuple->key[0] = '\0';
  tuple->type = etf_nil;
  tuple->count = 0;
  if (tuple->list) free(tuple->list);
  tuple->list = NULL;
  if (tuple->blob) free(tuple->blob);
  tuple->blob = NULL;
  tuple->len = 0;

  uint8_t one;
  uint32_t four;
  uint16_t two;

  int r;
  
  int n;
  // match magic
  if ((n = readb1(fd, &one)) <= 0) return n;
  if (one != ETF_MAGIC) return -etf_read_bad_magic;
  // match small tuple
  if ((n = readb1(fd, &one)) <= 0) return n;
  if (one != ETF_SMALL_TUPLE) return -etf_read_not_tuple;
  // get tuple len
  if ((n = readb1(fd, &one)) <= 0) return n;
  if (one > 2) return -etf_read_bad_tuple_len;
  tuple->count = one;
  LOG("tuple->count = %d"CR, tuple->count);
  if (tuple->count == 0) return etf_read_okay;
  if (one > 2) return -etf_read_bad_tuple_len;
  // match bin
  if ((n = readb1(fd, &one)) <= 0) return n;
  if (one != ETF_BINARY) return -etf_read_bad_ext;
  // match 4 byte len
  if ((n = readb4(fd, &four)) <= 0) return n;
  uint32_t len = four;
  if (len > KEY_MAX) return -etf_read_bad_len;
  if ((n = readbn(fd, tuple->key, len)) <= 0) return n;
  if (n != len) return -etf_read_bad_len;

  // make sure key is null-terminated
  tuple->key[len] = '\0';

  if (tuple->count == 1) return etf_read_okay;

  // next we look for...
  //
  // ETF_NIL = no list follows
  // ETF_SMALL_INTEGER = one uint8_t
  // ETF_INTEGER = one int32_t
  // ETF_BINARY = binary blob of uint8_t
  // ETF_STRING = simple list up to 65535 uint8_t
  // ETF_LIST = list of either uint8_t or int32_t
  
  if ((n = readb1(fd, &one)) <= 0) return n;

  switch (one) {
    case ETF_NIL:
      tuple->type = etf_nil;
      return etf_read_okay;
    case ETF_SMALL_INTEGER:
      if ((n = readb1(fd, &one)) <= 0) return n;
      tuple->type = etf_int;
      tuple->val = one;
      return etf_read_okay;
    case ETF_INTEGER:
      if ((n = readb4(fd, &four)) <= 0) return n;
      tuple->type = etf_int;
      tuple->val = four;
      return etf_read_okay;
    case ETF_BINARY:
      if ((n = readb4(fd, &four)) <= 0) return n;
      len = four;
      if (len) {
        tuple->blob = (uint8_t *)malloc((len+1) * sizeof(uint8_t));
        if (tuple->blob) {
          n = readbn(fd, tuple->blob, len);
          if (n <= 0) {
            free(tuple->blob);
            tuple->blob = NULL;
            return n;
          }
          if (n != len) {
            free(tuple->blob);
            tuple->blob = NULL;
            return -etf_read_bad_len;
          }
          tuple->type = etf_binary;
          tuple->len = len;
          tuple->blob[len] = '\0';
          return etf_read_okay;
        }
      }
      break;
    case ETF_STRING:
      if ((n = readb2(fd, &two)) <= 0) return n;
      len = two;
      if (len) {
        tuple->list = (int32_t *)malloc(len * sizeof(int32_t));
        if (tuple->list) {
          for (int i=0; i<len; i++) {
            if ((n = readb1(fd, &one)) <= 0) {
              free_list_fail(tuple, -etf_read_bad_val);
              return n;
            }
            if ((n = readb1(fd, &one)) <= 0) {
              free_list_fail(tuple, -etf_read_bad_val);
              return n;
            }
            tuple->list[i] = one;
          }
          tuple->type = etf_list;
          tuple->len = len;
          return etf_read_okay;
        }
      }
      break;
    case ETF_LIST:
      if ((n = readb4(fd, &four)) <= 0) return n;
      len = four;
      if (len) {
        tuple->list = (int32_t *)malloc(len * sizeof(int32_t));
        if (tuple->list) {
          for (int i=0; i<len; i++) {
            int32_t num = 0;
            // get type
            if ((n = readb1(fd, &one)) <= 0) {
              free_list_fail(tuple, -etf_read_bad_ext);
              return n;
            }
            // take next step based on type
            if (one == ETF_SMALL_INTEGER) {
              if ((n = readb1(fd, &one)) <= 0) {
                free_list_fail(tuple, -etf_read_bad_val);
                return n;
              }
              num = one;
            } else if (one == ETF_INTEGER) {
              if (readb4(fd, &four) <= 0) {
                free_list_fail(tuple, -etf_read_bad_val);
                return n;
              }
              num = four;
            } else {
              LOG("unexpected %d"CR, one);
              free_list_fail(tuple, -etf_read_bad_ext);
              return -etf_read_bad_ext;
            }
            LOG("[%d] = %d"CR, i, num);
            tuple->list[i] = num;
          }
          // get end of list nil
          if ((n = readb1(fd, &one)) <= 0) {
            free_list_fail(tuple, -etf_read_bad_ext);
            return n;
          }
          if (one != ETF_NIL) {
            free_list_fail(tuple, -etf_read_bad_ext);
            return -etf_read_bad_ext;
          }
          tuple->type = etf_list;
          tuple->len = len;
          return etf_read_okay;
        }
      }
      break;
    default:
      LOG("unexpected %d"CR, one);
      return -etf_read_bad_ext;
  }
  return -etf_read_unknown;
}

void cleaner(void) {
  LOG("cleaner()"CR);
}