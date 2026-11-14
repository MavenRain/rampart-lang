/* Span pins (S5): Q-S0-16 (a), Q-S5-1 (a) to Q-S5-3 (a), design sec 7.
   pins/spans.tsv names each base item with the sha256 of its exact line
   range at the pin of telcoin-network. */
#ifndef RAMPART_PINS_H
#define RAMPART_PINS_H
#include <stddef.h>

typedef enum {
  PINS_OK,
  PINS_RANGE,  /* the span ends after the last line of the file */
  PINS_FORMAT, /* start < 1, end < start, or a value that cannot be quoted */
  PINS_IO,     /* git failed: no checkout, no object or no path */
  PINS_SIZE,   /* the file is larger than the read cap */
} PinsStatus;

/* The sha256 of lines START..END (1-based, inclusive) of TEXT. Each line
   includes its own newline. A last line without a newline counts. */
PinsStatus pins_span_hash(const char *text, size_t len, long start, long end, unsigned char digest[32]);

/* 64 lowercase hex digits and a NUL. */
void pins_hex(const unsigned char digest[32], char out[65]);

/* The gate line when TN (the value of RAMPART_TN) is unset or empty, else
   NULL (Q-S5-2 (a)). */
const char *pins_skip_line(const char *tn);

/* Q-S5-1 (a): the lines that declare NAME in TEXT. NAME is `T::m` (an `fn m`
   inside an `impl` block that names T) or a bare name (`struct`, `enum`,
   `const`, `static` or `fn`, outside each `impl` block). A one-line impl
   (`impl Eq for S {}`) opens no block.
   The text after the first space of NAME is a note and is ignored. LINES
   gets the first MAX hits. The result is the count of all hits. */
size_t pins_find_decl(const char *text, size_t len, const char *name, long *lines, size_t max);

/* `langc pins ...`: ARGV[0] is "pins". */
int pins_main(int argc, char **argv);
#endif
