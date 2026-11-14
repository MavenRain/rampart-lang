/* SHA-256 (FIPS 180-4) for the span pins of `langc pins` (S5, Q-S0-16).
   A copy of relay-lang 073cde7 test/sha256.h, which is a copy of
   cairn-lang src/sha256.h. The other comments are the cairn text. */
#ifndef LANG_SHA256_H
#define LANG_SHA256_H
#include <stddef.h>
#include <stdint.h>

void sha256(const unsigned char *data, size_t size, unsigned char digest[32]);
/* The round constants K (section 4.2.2) and the initial hash H(0) (section
   5.3.3). */
extern const uint32_t sha256_round_constant[64];
extern const uint32_t sha256_initial_hash[8];
#endif
