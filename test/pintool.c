/* C tests of the span pins (S5, ruling 2026-10-09): the sha256 vectors,
   a span hash over a fixture file, a CHANGED span, a span past the end of
   the file, the SKIPPED gate line (Q-S5-2 (a)) and the bump search
   (Q-S5-1 (a)), with a one-line impl and the words that are not
   keywords. Run from the repo root (make check). */
#include <stdio.h>
#include <string.h>

#include "pins.h"
#include "sha256.h"

#define FIXTURE_MAX 4096
#define FIPS_56 "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
/* Line 1 is a one-line impl, line 4 a free fn in a module, lines 7 and 8
   use `type` and `trait`, which are not bump keywords (Q-S5-1 (a)). */
#define ONE_LINE_IMPL                                                                                                 \
  "impl Eq for S {}\n\nmod m {\n    pub fn helper() {}\n}\n\npub type N = u32;\npub trait Tr {}\n"

static int passed = 0;
static int failed = 0;

static void expect(int ok, const char *what) {
  passed += ok;
  failed += !ok;
  if (!ok) printf("FAIL pins %s\n", what);
}

static int hex_is(const char *text, size_t len, const char *want) {
  unsigned char digest[32];
  char hex[65];
  sha256((const unsigned char *)text, len, digest);
  pins_hex(digest, hex);
  return strcmp(hex, want) == 0;
}

/* The bytes of PATH in BUF (NUL-terminated), or 0. */
static size_t load(const char *path, char *buf) {
  FILE *file = fopen(path, "rb");
  size_t count;
  if (file == NULL) return 0;
  count = fread(buf, 1, FIXTURE_MAX - 1, file);
  fclose(file);
  buf[count] = '\0';
  return count;
}

/* 1 when the span hash of lines S..E of TEXT is the sha256 of BYTES. */
static int span_is(const char *text, size_t len, long s, long e, const char *bytes) {
  unsigned char got[32];
  unsigned char want[32];
  if (pins_span_hash(text, len, s, e, got) != PINS_OK) return 0;
  sha256((const unsigned char *)bytes, strlen(bytes), want);
  return memcmp(got, want, 32) == 0;
}

/* 1 when the spans S..E of A and SB..EB of B have the same hash. */
static int spans_equal(const char *a, size_t alen, long s, long e, const char *b, size_t blen, long sb, long eb) {
  unsigned char x[32];
  unsigned char y[32];
  return pins_span_hash(a, alen, s, e, x) == PINS_OK && pins_span_hash(b, blen, sb, eb, y) == PINS_OK &&
         memcmp(x, y, 32) == 0;
}

/* 1 when NAME has exactly one hit in TEXT, at line WANT. */
static int found_at(const char *text, size_t len, const char *name, long want) {
  long lines[4] = {0, 0, 0, 0};
  return pins_find_decl(text, len, name, lines, 4) == 1 && lines[0] == want;
}

int main(void) {
  static char a[FIXTURE_MAX];
  static char b[FIXTURE_MAX];
  static char c[FIXTURE_MAX];
  size_t alen = load("test/pins/a.rs", a);
  size_t blen = load("test/pins/b.rs", b);
  unsigned char digest[32];
  long lines[4] = {0, 0, 0, 0};
  const char *skip = pins_skip_line(NULL);
  char *at;

  expect(hex_is("", 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"), "sha256 of the empty string");
  expect(hex_is("abc", 3, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), "sha256 of abc");
  expect(hex_is(FIPS_56, 56, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"),
         "sha256 of the 56-byte FIPS 180-2 vector");
  expect(alen > 0 && blen > 0, "the fixtures load");

  /* A span is its lines, each with its own newline. */
  expect(span_is(a, alen, 2, 4, "pub struct Score {\n    value: u32,\n}\n"), "span hash of a.rs lines 2-4");
  expect(span_is("x\ny", 3, 2, 2, "y"), "span hash of a last line without a newline");

  /* CHANGED: a changed byte in the range changes the hash; a changed byte
     out of the range does not. */
  memcpy(c, a, alen + 1);
  at = strstr(c, "value: u32");
  if (at != NULL) at[0] = 'V';
  expect(at != NULL && !spans_equal(a, alen, 2, 4, c, alen, 2, 4), "a changed byte in the span gives CHANGED");
  memcpy(c, a, alen + 1);
  at = strstr(c, "LIMIT");
  if (at != NULL) at[0] = 'X';
  expect(at != NULL && spans_equal(a, alen, 2, 4, c, alen, 2, 4), "a changed byte out of the span gives SAME");

  /* Refusals, not crashes. */
  expect(pins_span_hash(a, alen, 11, 99, digest) == PINS_RANGE, "a span past the end of the file is PINS_RANGE");
  expect(pins_span_hash(a, alen, 13, 13, digest) == PINS_RANGE, "a span after the last newline is PINS_RANGE");
  expect(pins_span_hash("", 0, 1, 1, digest) == PINS_RANGE, "a span of an empty file is PINS_RANGE");
  expect(pins_span_hash(a, alen, 0, 2, digest) == PINS_FORMAT, "start 0 is PINS_FORMAT");
  expect(pins_span_hash(a, alen, 4, 2, digest) == PINS_FORMAT, "end before start is PINS_FORMAT");

  /* The gate line (Q-S5-2 (a)). */
  expect(skip != NULL && strcmp(skip, "pins: SKIPPED (no RAMPART_TN)") == 0, "unset RAMPART_TN gives SKIPPED");
  expect(pins_skip_line("") != NULL, "empty RAMPART_TN gives SKIPPED");
  expect(pins_skip_line("/x") == NULL, "a set RAMPART_TN does not skip");

  /* bump (Q-S5-1 (a)): b.rs is a.rs after an upstream bump. */
  expect(found_at(a, alen, "Score", 2), "find struct Score in a.rs");
  expect(found_at(b, blen, "Score", 5), "find the moved struct Score in b.rs");
  expect(found_at(b, blen, "Score (note)", 5), "the note after a space is not part of the name");
  expect(found_at(b, blen, "Score::apply_penalty", 14), "find Score::apply_penalty, not Other::apply_penalty");
  expect(found_at(b, blen, "Other::apply_penalty", 10), "find Other::apply_penalty");
  expect(found_at(b, blen, "LIMIT", 19), "find the moved const LIMIT");
  expect(spans_equal(a, alen, 2, 4, b, blen, 5, 7), "the moved struct has the same span hash");
  expect(!spans_equal(a, alen, 12, 12, b, blen, 19, 19), "the changed const has a new span hash");
  expect(pins_find_decl(b, blen, "helper", lines, 4) == 2 && lines[0] == 22 && lines[1] == 26,
         "two free fns helper are AMBIGUOUS");
  expect(pins_find_decl(b, blen, "apply_penalty", lines, 4) == 0, "a method is not a bare fn");
  expect(pins_find_decl(b, blen, "GONE", lines, 4) == 0, "a removed item is MISSING");
  expect(pins_find_decl(b, blen, "Score::gone", lines, 4) == 0, "a removed method is MISSING");

  /* A one-line impl opens no block, so the fn after it is a free fn. */
  expect(found_at(ONE_LINE_IMPL, strlen(ONE_LINE_IMPL), "helper", 4), "a one-line impl opens no block");
  expect(pins_find_decl(ONE_LINE_IMPL, strlen(ONE_LINE_IMPL), "S::helper", lines, 4) == 0,
         "a fn after a one-line impl is not a method");
  expect(pins_find_decl(ONE_LINE_IMPL, strlen(ONE_LINE_IMPL), "N", lines, 4) == 0, "a type line is not a hit");
  expect(pins_find_decl(ONE_LINE_IMPL, strlen(ONE_LINE_IMPL), "Tr", lines, 4) == 0, "a trait line is not a hit");

  printf(failed == 0 ? "pin tests: %d passed\n" : "pin tests: %d passed, %d failed\n", passed, failed);
  return failed == 0 ? 0 : 1;
}
