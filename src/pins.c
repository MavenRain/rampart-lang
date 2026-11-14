/* Span pins (S5): `langc pins hash|verify|bump|gate`. Q-S0-16 (a),
   Q-S5-1 (a), Q-S5-2 (a), Q-S5-3 (a) and design sec 7.
   pins/spans.tsv: the line `# pin SHA`, then one TAB-separated row for each
   base name: `name path start end sha256`. Other lines that start with `#`
   are comments. The hash is the sha256 of lines start..end of the file at
   the pin (pins_span_hash). telcoin-network is read only through
   `git -C DIR show SHA:PATH`.
   Exit 0: ok. Exit 1: a refused span, a CHANGED row or a gate FAIL.
   Exit 2: usage or IO. */
#define _POSIX_C_SOURCE 200809L
#include "pins.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sha256.h"

#define PINS_FILE_MAX ((size_t)4 << 20)
#define PINS_SPANS_MAX ((size_t)1 << 20)
#define PINS_ROW_MAX 256
#define PINS_FIELD_MAX 256
#define PINS_LINE_MAX 10000000L
#define PINS_HITS_MAX 8
#define PINS_INDENT_MAX 64
#define PINS_CMD_MAX 2048
#define PINS_KEYWORDS 5
#define PINS_NEEDLE_MAX (PINS_FIELD_MAX + 8)
#define PINS_HEX "0123456789abcdef"

typedef struct {
  char name[PINS_FIELD_MAX];
  char path[PINS_FIELD_MAX];
  long start;
  long end;
  char sha[65];
} PinsRow;

typedef struct {
  char pin[41];
  PinsRow rows[PINS_ROW_MAX];
  size_t count;
} PinsTable;

typedef enum {
  PINS_CMD_HASH,
  PINS_CMD_VERIFY,
  PINS_CMD_BUMP,
  PINS_CMD_GATE,
} PinsCommand;

typedef struct {
  size_t at;
  size_t len;
  int set;
} ImplLine;

static const struct {
  const char *word;
  PinsCommand command;
} PINS_COMMANDS[] = {
  {"hash", PINS_CMD_HASH},
  {"verify", PINS_CMD_VERIFY},
  {"bump", PINS_CMD_BUMP},
  {"gate", PINS_CMD_GATE},
};

static PinsTable table;

static int pins_usage(void) {
  fputs("usage: langc pins hash --tn DIR [--spans FILE]\n"
        "       langc pins verify --tn DIR [--spans FILE]\n"
        "       langc pins bump NEWSHA --tn DIR [--spans FILE]\n"
        "       langc pins gate [--spans FILE]\n"
        "FILE is pins/spans.tsv when not given. gate reads the checkout from RAMPART_TN.\n",
        stderr);
  return 2;
}

/* Sets *OUT to the offset of the first byte of line LINE (1-based). 0 when
   TEXT has fewer lines. */
static int line_offset(const char *text, size_t len, long line, size_t *out) {
  long at = 1;
  size_t i = 0;
  for (; i < len && at < line; i++) at += text[i] == '\n';
  if (at != line || i >= len) return 0;
  *out = i;
  return 1;
}

/* The offset after the newline of the line that starts at FROM, or LEN when
   that line has no newline. */
static size_t line_stop(const char *text, size_t len, size_t from) {
  size_t i = from;
  for (; i < len && text[i] != '\n'; i++) {
  }
  return i < len ? i + 1 : len;
}

PinsStatus pins_span_hash(const char *text, size_t len, long start, long end, unsigned char digest[32]) {
  size_t begin = 0;
  size_t last = 0;
  if (start < 1 || end < start) return PINS_FORMAT;
  if (!line_offset(text, len, start, &begin) || !line_offset(text, len, end, &last)) return PINS_RANGE;
  sha256((const unsigned char *)text + begin, line_stop(text, len, last) - begin, digest);
  return PINS_OK;
}

void pins_hex(const unsigned char digest[32], char out[65]) {
  for (size_t i = 0; i < 32; i++) {
    out[2 * i] = PINS_HEX[digest[i] >> 4];
    out[2 * i + 1] = PINS_HEX[digest[i] & 15u];
  }
  out[64] = '\0';
}

const char *pins_skip_line(const char *tn) {
  return tn == NULL || tn[0] == '\0' ? "pins: SKIPPED (no RAMPART_TN)" : NULL;
}

static int is_ident(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

/* 1 when WORD is in LINE[0..LEN) with no identifier character on either
   side. */
static int has_word(const char *line, size_t len, const char *word) {
  size_t wlen = strlen(word);
  for (size_t i = 0; wlen > 0 && i + wlen <= len; i++) {
    int left = i == 0 || !is_ident(line[i - 1]);
    int right = i + wlen == len || !is_ident(line[i + wlen]);
    if (left && right && memcmp(line + i, word, wlen) == 0) return 1;
  }
  return 0;
}

static int starts_word(const char *text, size_t len, const char *word) {
  size_t wlen = strlen(word);
  return len >= wlen && memcmp(text, word, wlen) == 0 && (len == wlen || !is_ident(text[wlen]));
}

/* The innermost `impl` line that is open at INDENT, or NULL. */
static const ImplLine *open_impl(const ImplLine *impls, size_t indent) {
  const ImplLine *found = NULL;
  for (size_t k = 0; k < indent && k < PINS_INDENT_MAX; k++) found = impls[k].set ? &impls[k] : found;
  return found;
}

/* 1 when the code line BODY at INDENT declares the item. OWNER is T for
   `T::m`, else NULL. */
static int is_decl(const char *text, const ImplLine *impls, size_t indent, const char *body, size_t body_len,
                   char needles[][PINS_NEEDLE_MAX], size_t needle_count, const char *owner) {
  const ImplLine *impl = open_impl(impls, indent);
  int named = 0;
  for (size_t k = 0; k < needle_count && !named; k++) named = has_word(body, body_len, needles[k]);
  if (!named) return 0;
  if (owner == NULL) return impl == NULL;
  return impl != NULL && has_word(text + impl->at, impl->len, owner);
}

size_t pins_find_decl(const char *text, size_t len, const char *name, long *lines, size_t max) {
  static const char *const KEYWORDS[PINS_KEYWORDS] = {"struct", "enum", "const", "static", "fn"};
  char base[PINS_FIELD_MAX];
  char needles[PINS_KEYWORDS][PINS_NEEDLE_MAX];
  ImplLine impls[PINS_INDENT_MAX];
  size_t base_len = strcspn(name, " ");
  size_t needle_count = PINS_KEYWORDS;
  size_t hits = 0;
  long line = 1;
  char *sep;
  const char *owner = NULL;
  if (base_len == 0 || base_len >= sizeof base) return 0;
  memcpy(base, name, base_len);
  base[base_len] = '\0';
  sep = strstr(base, "::");
  if (sep != NULL) {
    sep[0] = '\0';
    owner = base;
    needle_count = 1;
    snprintf(needles[0], sizeof needles[0], "fn %s", sep + 2);
  }
  for (size_t k = 0; sep == NULL && k < PINS_KEYWORDS; k++) {
    snprintf(needles[k], sizeof needles[k], "%s %s", KEYWORDS[k], base);
  }
  memset(impls, 0, sizeof impls);
  for (size_t at = 0; at < len; line++) {
    size_t stop = line_stop(text, len, at);
    size_t end = text[stop - 1] == '\n' ? stop - 1 : stop;
    size_t indent = 0;
    for (; at + indent < end && text[at + indent] == ' '; indent++) {
    }
    const char *body = text + at + indent;
    size_t body_len = end - at - indent;
    int code = body_len > 0 && !(body_len >= 2 && body[0] == '/' && body[1] == '/') && indent < PINS_INDENT_MAX;
    int closes = code && body[0] == '}';
    /* A one-line impl (`impl Eq for S {}`) opens no block. */
    int opens = code && starts_word(body, body_len, "impl") && memchr(body, '}', body_len) == NULL;
    for (size_t k = indent; closes && k < PINS_INDENT_MAX; k++) impls[k].set = 0;
    if (opens) {
      impls[indent].at = at;
      impls[indent].len = end - at;
      impls[indent].set = 1;
    }
    if (code && !opens && is_decl(text, impls, indent, body, body_len, needles, needle_count, owner)) {
      if (hits < max) lines[hits] = line;
      hits++;
    }
    at = stop;
  }
  return hits;
}

static int safe_dir(const char *dir) {
  return dir != NULL && dir[0] != '\0' && strchr(dir, '\'') == NULL;
}

static int safe_sha(const char *sha) {
  size_t n = strlen(sha);
  return n >= 4 && n <= 40 && strspn(sha, PINS_HEX) == n;
}

static int safe_path(const char *path) {
  size_t n = strlen(path);
  return n > 0 && path[0] != '/' && strstr(path, "..") == NULL &&
         strspn(path, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_./-") == n;
}

/* Runs CMD through popen. When OUT is not NULL, *OUT gets its stdout (the
   caller frees it). */
static PinsStatus run_git(const char *cmd, char **out, size_t *len) {
  FILE *pipe = popen(cmd, "r");
  char *buf;
  size_t count;
  int status;
  if (pipe == NULL) return PINS_IO;
  if (out == NULL) return pclose(pipe) == 0 ? PINS_OK : PINS_IO;
  buf = malloc(PINS_FILE_MAX + 1);
  count = buf == NULL ? 0 : fread(buf, 1, PINS_FILE_MAX + 1, pipe);
  status = pclose(pipe);
  if (buf == NULL) return PINS_IO;
  /* Test the size first: git gets SIGPIPE when the file is too large. */
  if (count > PINS_FILE_MAX || status != 0) {
    free(buf);
    return count > PINS_FILE_MAX ? PINS_SIZE : PINS_IO;
  }
  *out = buf;
  *len = count;
  return PINS_OK;
}

static PinsStatus git_show(const char *dir, const char *sha, const char *path, char **out, size_t *len) {
  char cmd[PINS_CMD_MAX];
  int n;
  if (!safe_dir(dir) || !safe_sha(sha) || !safe_path(path)) return PINS_FORMAT;
  n = snprintf(cmd, sizeof cmd, "git -C '%s' show '%s:%s' 2>/dev/null", dir, sha, path);
  if (n < 0 || (size_t)n >= sizeof cmd) return PINS_FORMAT;
  return run_git(cmd, out, len);
}

/* 1 when the checkout DIR has the commit SHA. */
static int has_commit(const char *dir, const char *sha) {
  char cmd[PINS_CMD_MAX];
  int n;
  if (!safe_dir(dir) || !safe_sha(sha)) return 0;
  n = snprintf(cmd, sizeof cmd, "git -C '%s' cat-file -e '%s^{commit}' 2>/dev/null", dir, sha);
  return n >= 0 && (size_t)n < sizeof cmd && run_git(cmd, NULL, NULL) == PINS_OK;
}

/* The exit status for a failed span read or hash of ROW at SHA. */
static int report(PinsStatus status, const PinsRow *row, const char *sha) {
  switch (status) {
    case PINS_OK:
      return 0;
    case PINS_RANGE:
      fprintf(stderr, "SPAN_RANGE %s: %s:%ld-%ld ends after the last line of the file at %s\n", row->name,
              row->path, row->start, row->end, sha);
      return 1;
    case PINS_FORMAT:
      fprintf(stderr, "SPANS_FORMAT %s: the directory, SHA or path %s cannot be quoted for git\n", row->name,
              row->path);
      return 2;
    case PINS_IO:
      fprintf(stderr, "IO %s: git cannot show %s:%s\n", row->name, sha, row->path);
      return 2;
    case PINS_SIZE:
      fprintf(stderr, "IO_SIZE %s: %s at %s is larger than %zu bytes\n", row->name, row->path, sha, PINS_FILE_MAX);
      return 2;
  }
  return 2;
}

static int copy_field(char *dst, size_t cap, const char *src) {
  size_t n = strlen(src);
  if (n == 0 || n >= cap) return 0;
  memcpy(dst, src, n + 1);
  return 1;
}

static int parse_line_number(const char *text, long *out) {
  char *stop = NULL;
  long value = text[0] >= '0' && text[0] <= '9' ? strtol(text, &stop, 10) : 0;
  if (stop == NULL || stop == text || *stop != '\0' || value < 1 || value > PINS_LINE_MAX) return 0;
  *out = value;
  return 1;
}

/* One row: 4 fields (the input of `hash`) or 5 (name path start end
   sha256). */
static int parse_row(char *line, const char *file, size_t number, PinsRow *row) {
  char *fields[6];
  size_t count = 1;
  int ok;
  fields[0] = line;
  for (char *tab = strchr(line, '\t'); tab != NULL && count < 6; tab = strchr(tab + 1, '\t')) {
    tab[0] = '\0';
    fields[count++] = tab + 1;
  }
  ok = (count == 4 || count == 5) && copy_field(row->name, sizeof row->name, fields[0]) &&
       copy_field(row->path, sizeof row->path, fields[1]) && safe_path(row->path) &&
       parse_line_number(fields[2], &row->start) && parse_line_number(fields[3], &row->end) &&
       row->start <= row->end;
  row->sha[0] = '\0';
  if (ok && count == 5) ok = strlen(fields[4]) == 64 && strspn(fields[4], PINS_HEX) == 64;
  if (ok && count == 5) memcpy(row->sha, fields[4], 65);
  if (!ok) {
    fprintf(stderr, "SPANS_FORMAT %s line %zu: want name, path, start <= end and an optional sha256, TAB-separated\n",
            file, number);
  }
  return ok;
}

static int is_duplicate(const char *name) {
  for (size_t i = 0; i < table.count; i++) {
    if (strcmp(table.rows[i].name, name) == 0) return 1;
  }
  return 0;
}

static int parse_header(const char *line, const char *file) {
  int ok = strncmp(line, "# pin ", 6) == 0 && strlen(line + 6) == 40 && strspn(line + 6, PINS_HEX) == 40;
  if (ok) memcpy(table.pin, line + 6, 41);
  if (!ok) fprintf(stderr, "SPANS_FORMAT %s line 1: want `# pin SHA` with 40 hex digits\n", file);
  return ok;
}

static int parse_table(const char *text, size_t len, const char *file) {
  size_t number = 0;
  table.count = 0;
  table.pin[0] = '\0';
  for (size_t at = 0; at < len;) {
    size_t stop = line_stop(text, len, at);
    size_t end = text[stop - 1] == '\n' ? stop - 1 : stop;
    char line[2 * PINS_FIELD_MAX + 128];
    number++;
    if (end - at >= sizeof line) {
      fprintf(stderr, "SPANS_FORMAT %s line %zu: the line is too long\n", file, number);
      return 0;
    }
    memcpy(line, text + at, end - at);
    line[end - at] = '\0';
    at = stop;
    if (number == 1 && !parse_header(line, file)) return 0;
    if (number == 1 || line[0] == '\0' || line[0] == '#') continue;
    if (table.count >= PINS_ROW_MAX) {
      fprintf(stderr, "SPANS_FORMAT %s line %zu: more than %d rows\n", file, number, PINS_ROW_MAX);
      return 0;
    }
    if (!parse_row(line, file, number, &table.rows[table.count])) return 0;
    if (is_duplicate(table.rows[table.count].name)) {
      fprintf(stderr, "SPANS_FORMAT %s line %zu: the name %s has a row already\n", file, number,
              table.rows[table.count].name);
      return 0;
    }
    table.count++;
  }
  if (number == 0) fprintf(stderr, "SPANS_FORMAT %s: the file is empty\n", file);
  return number > 0;
}

static int load_table(const char *file) {
  FILE *in = fopen(file, "rb");
  char *buf;
  size_t count;
  int read_error;
  int ok;
  if (in == NULL) {
    fprintf(stderr, "IO cannot open %s\n", file);
    return 0;
  }
  buf = malloc(PINS_SPANS_MAX + 1);
  count = buf == NULL ? 0 : fread(buf, 1, PINS_SPANS_MAX + 1, in);
  read_error = ferror(in);
  fclose(in);
  if (buf == NULL || read_error || count > PINS_SPANS_MAX) {
    fprintf(stderr, "IO cannot read %s (at most %zu bytes)\n", file, PINS_SPANS_MAX);
    free(buf);
    return 0;
  }
  ok = parse_table(buf, count, file);
  free(buf);
  return ok;
}

/* The hash of ROW at SHA in the checkout DIR. */
static PinsStatus row_hash(const char *dir, const char *sha, const PinsRow *row, char hex[65]) {
  char *text = NULL;
  size_t len = 0;
  unsigned char digest[32];
  PinsStatus status = git_show(dir, sha, row->path, &text, &len);
  if (status != PINS_OK) return status;
  status = pins_span_hash(text, len, row->start, row->end, digest);
  free(text);
  if (status == PINS_OK) pins_hex(digest, hex);
  return status;
}

/* Q-S5-3 (a): prints each row with its hash at the pin. */
static int run_hash(const char *dir) {
  /* Hash each row first, so a refusal prints no partial table. */
  for (size_t i = 0; i < table.count; i++) {
    PinsRow *row = &table.rows[i];
    char hex[65];
    PinsStatus status = row_hash(dir, table.pin, row, hex);
    if (status != PINS_OK) return report(status, row, table.pin);
    snprintf(row->sha, sizeof row->sha, "%s", hex);
  }
  printf("# pin %s\n# name\tpath\tstart\tend\tsha256 (written by langc pins hash)\n", table.pin);
  for (size_t i = 0; i < table.count; i++) {
    const PinsRow *row = &table.rows[i];
    printf("%s\t%s\t%ld\t%ld\t%s\n", row->name, row->path, row->start, row->end, row->sha);
  }
  return 0;
}

/* SAME or CHANGED for each row at the pin. The gate form prints only the
   CHANGED rows and the count line. */
static int run_verify(const char *dir, int gate) {
  size_t same = 0;
  if (table.count == 0) {
    puts("FAIL pins: the table has no rows");
    return 1;
  }
  for (size_t i = 0; i < table.count; i++) {
    const PinsRow *row = &table.rows[i];
    char hex[65] = "";
    PinsStatus status = row_hash(dir, table.pin, row, hex);
    int is_same = status == PINS_OK && strcmp(hex, row->sha) == 0;
    if (status != PINS_OK && status != PINS_RANGE) return report(status, row, table.pin);
    same += is_same;
    if (is_same && !gate) printf("SAME %s\n", row->name);
    if (!is_same) printf("CHANGED %s %s:%ld-%ld\n", row->name, row->path, row->start, row->end);
  }
  printf("pins: %zu SAME, %zu CHANGED\n", same, table.count - same);
  return same == table.count ? 0 : 1;
}

/* Q-S5-2 (a): SKIPPED when RAMPART_TN is unset or empty, FAIL when the
   checkout has no pin object, else the gate form of verify. */
static int run_gate(void) {
  const char *tn = getenv("RAMPART_TN");
  const char *skip = pins_skip_line(tn);
  if (skip != NULL) {
    puts(skip);
    return 0;
  }
  if (!has_commit(tn, table.pin)) {
    printf("FAIL pins: RAMPART_TN has no %.9s\n", table.pin);
    return 1;
  }
  return run_verify(tn, 1);
}

static void print_missing(const PinsRow *row, size_t counts[4]) {
  printf("MISSING\t%s\t%s\t-\t-\t-\n", row->name, row->path);
  counts[3]++;
}

/* Q-S5-1 (a): one row for ROW in the file TEXT at the new SHA. COUNTS:
   SAME, FOUND, AMBIGUOUS, MISSING. */
static void bump_row(const PinsRow *row, const char *text, size_t len, size_t counts[4]) {
  unsigned char digest[32];
  char hex[65] = "";
  long lines[PINS_HITS_MAX];
  size_t hits;
  long start;
  long end;
  if (pins_span_hash(text, len, row->start, row->end, digest) == PINS_OK) pins_hex(digest, hex);
  if (strcmp(hex, row->sha) == 0) {
    printf("SAME\t%s\t%s\t%ld\t%ld\t%s\n", row->name, row->path, row->start, row->end, hex);
    counts[0]++;
    return;
  }
  hits = pins_find_decl(text, len, row->name, lines, PINS_HITS_MAX);
  if (hits == 0) {
    print_missing(row, counts);
    return;
  }
  if (hits > 1) {
    printf("AMBIGUOUS\t%s\t%s\t", row->name, row->path);
    for (size_t i = 0; i < hits && i < PINS_HITS_MAX; i++) printf("%s%ld", i == 0 ? "" : ",", lines[i]);
    printf("%s\t-\t-\n", hits > PINS_HITS_MAX ? ",..." : "");
    counts[2]++;
    return;
  }
  start = lines[0];
  end = start + (row->end - row->start);
  hex[0] = '\0';
  if (pins_span_hash(text, len, start, end, digest) == PINS_OK) pins_hex(digest, hex);
  printf("FOUND\t%s\t%s\t%ld\t%ld\t%s\n", row->name, row->path, start, end, hex[0] == '\0' ? "-" : hex);
  counts[1]++;
}

static int run_bump(const char *dir, const char *newsha) {
  size_t counts[4] = {0, 0, 0, 0};
  if (!has_commit(dir, newsha)) {
    fprintf(stderr, "IO the checkout %s has no commit %s\n", dir, newsha);
    return 2;
  }
  printf("# pin %s\n", newsha);
  for (size_t i = 0; i < table.count; i++) {
    const PinsRow *row = &table.rows[i];
    char *text = NULL;
    size_t len = 0;
    PinsStatus status = git_show(dir, newsha, row->path, &text, &len);
    if (status == PINS_FORMAT || status == PINS_SIZE) return report(status, row, newsha);
    if (status == PINS_IO) print_missing(row, counts);
    if (status == PINS_OK) bump_row(row, text, len, counts);
    free(text);
  }
  printf("# bump: %zu SAME, %zu FOUND, %zu AMBIGUOUS, %zu MISSING\n", counts[0], counts[1], counts[2], counts[3]);
  return 0;
}

static int all_hashed(const char *file) {
  for (size_t i = 0; i < table.count; i++) {
    if (table.rows[i].sha[0] == '\0') {
      fprintf(stderr, "SPANS_FORMAT %s: the row %s has no sha256 (run langc pins hash)\n", file,
              table.rows[i].name);
      return 0;
    }
  }
  return 1;
}

int pins_main(int argc, char **argv) {
  PinsCommand command = PINS_CMD_GATE;
  const char *dir = NULL;
  const char *spans = "pins/spans.tsv";
  const char *newsha = NULL;
  int found = 0;
  int first;
  for (size_t i = 0; argc > 1 && i < sizeof PINS_COMMANDS / sizeof PINS_COMMANDS[0]; i++) {
    int same = strcmp(PINS_COMMANDS[i].word, argv[1]) == 0;
    command = same ? PINS_COMMANDS[i].command : command;
    found |= same;
  }
  if (!found || (command == PINS_CMD_BUMP && argc < 3)) return pins_usage();
  first = command == PINS_CMD_BUMP ? 3 : 2;
  newsha = command == PINS_CMD_BUMP ? argv[2] : NULL;
  for (int i = first; i < argc; i += 2) {
    int has_value = i + 1 < argc;
    int is_tn = has_value && strcmp(argv[i], "--tn") == 0;
    int is_spans = has_value && strcmp(argv[i], "--spans") == 0;
    if (!is_tn && !is_spans) return pins_usage();
    dir = is_tn ? argv[i + 1] : dir;
    spans = is_spans ? argv[i + 1] : spans;
  }
  if ((command == PINS_CMD_GATE) != (dir == NULL)) return pins_usage();
  if (newsha != NULL && !safe_sha(newsha)) {
    fprintf(stderr, "USAGE bump needs a SHA of 4 to 40 lowercase hex digits\n");
    return 2;
  }
  if (!load_table(spans)) return 2;
  if (command != PINS_CMD_HASH && !all_hashed(spans)) return 2;
  switch (command) {
    case PINS_CMD_HASH:
      return run_hash(dir);
    case PINS_CMD_VERIFY:
      return run_verify(dir, 0);
    case PINS_CMD_BUMP:
      return run_bump(dir, newsha);
    case PINS_CMD_GATE:
      return run_gate();
  }
  return 2;
}
