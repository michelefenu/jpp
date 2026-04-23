/*
 * jpp - JSON Pretty Printer
 *
 * A minimal, dependency-free JSON pretty printer written in C99.
 * Reads JSON from stdin and writes formatted output to stdout.
 *
 * Usage:
 *   cat file.json | jpp
 *   jpp < file.json
 *   jpp file.json
 *
 * Options:
 *   -t N    Set indent width to N spaces (default: 2)
 *   -c      Enable colored output (auto-detected for TTY)
 *   -C      Disable colored output
 *   -h      Show help
 *
 * License: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#define JPP_VERSION "1.0.0"
#define DEFAULT_INDENT 2
#define READ_CHUNK 4096

/* ANSI color codes */
#define CLR_RESET   "\033[0m"
#define CLR_KEY     "\033[1;34m"   /* bold blue    */
#define CLR_STRING  "\033[0;32m"   /* green        */
#define CLR_NUMBER  "\033[0;33m"   /* yellow       */
#define CLR_BOOL    "\033[0;35m"   /* magenta      */
#define CLR_NULL    "\033[1;31m"   /* bold red     */
#define CLR_BRACE   "\033[1;37m"   /* bold white   */

typedef struct {
    char  *data;
    size_t len;
    size_t pos;
} Buffer;

typedef struct {
    int indent;
    int color;
} Options;

/* ---------- helpers ---------- */

static void die(const char *msg) {
    fprintf(stderr, "jpp: %s\n", msg);
    exit(1);
}

static void die_pos(const Buffer *b, const char *msg) {
    /* count line and column for error reporting */
    size_t line = 1, col = 1;
    for (size_t i = 0; i < b->pos && i < b->len; i++) {
        if (b->data[i] == '\n') { line++; col = 1; }
        else { col++; }
    }
    fprintf(stderr, "jpp: error at line %zu, col %zu: %s\n", line, col, msg);
    exit(1);
}

static char *read_all(FILE *fp, size_t *out_len) {
    size_t cap = READ_CHUNK, len = 0;
    char *buf = malloc(cap);
    if (!buf) die("out of memory");

    size_t n;
    while ((n = fread(buf + len, 1, READ_CHUNK, fp)) > 0) {
        len += n;
        if (len + READ_CHUNK > cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) { free(buf); die("out of memory"); }
            buf = tmp;
        }
    }
    buf[len] = '\0';
    *out_len = len;
    return buf;
}

/* ---------- lexer-level helpers ---------- */

static void skip_ws(Buffer *b) {
    while (b->pos < b->len) {
        char c = b->data[b->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            b->pos++;
        else
            break;
    }
}

static char peek(Buffer *b) {
    skip_ws(b);
    if (b->pos >= b->len) die_pos(b, "unexpected end of input");
    return b->data[b->pos];
}

static char advance(Buffer *b) {
    if (b->pos >= b->len) die_pos(b, "unexpected end of input");
    return b->data[b->pos++];
}

/* ---------- printing helpers ---------- */

static void put_indent(int depth, int width) {
    int n = depth * width;
    for (int i = 0; i < n; i++) putchar(' ');
}

static void put_color(const Options *o, const char *code) {
    if (o->color) fputs(code, stdout);
}

/* ---------- pretty-print engine ---------- */

static void pp_value(Buffer *b, int depth, const Options *o, int is_key);

static void pp_string(Buffer *b, int depth, const Options *o, int is_key) {
    (void)depth;
    put_color(o, is_key ? CLR_KEY : CLR_STRING);

    /* opening quote */
    if (advance(b) != '"') die_pos(b, "expected '\"'");
    putchar('"');

    /* body */
    while (b->pos < b->len) {
        char c = advance(b);
        if (c == '\\') {
            putchar('\\');
            if (b->pos < b->len) {
                putchar(advance(b));
            }
        } else if (c == '"') {
            putchar('"');
            put_color(o, CLR_RESET);
            return;
        } else {
            putchar(c);
        }
    }
    die_pos(b, "unterminated string");
}

static void pp_number(Buffer *b, const Options *o) {
    put_color(o, CLR_NUMBER);
    while (b->pos < b->len) {
        char c = b->data[b->pos];
        if (c == '-' || c == '+' || c == '.' ||
            (c >= '0' && c <= '9') ||
            c == 'e' || c == 'E') {
            putchar(c);
            b->pos++;
        } else {
            break;
        }
    }
    put_color(o, CLR_RESET);
}

static void pp_literal(Buffer *b, const Options *o,
                        const char *word, const char *color) {
    size_t wlen = strlen(word);
    if (b->pos + wlen > b->len ||
        memcmp(b->data + b->pos, word, wlen) != 0)
        die_pos(b, "invalid literal");
    put_color(o, color);
    fwrite(b->data + b->pos, 1, wlen, stdout);
    put_color(o, CLR_RESET);
    b->pos += wlen;
}

static void pp_object(Buffer *b, int depth, const Options *o) {
    b->pos++; /* skip '{' */
    put_color(o, CLR_BRACE);
    putchar('{');
    put_color(o, CLR_RESET);

    skip_ws(b);
    if (peek(b) == '}') {
        b->pos++;
        put_color(o, CLR_BRACE);
        putchar('}');
        put_color(o, CLR_RESET);
        return;
    }

    putchar('\n');
    int first = 1;
    while (1) {
        if (!first) {
            putchar(',');
            putchar('\n');
        }
        first = 0;

        skip_ws(b);
        put_indent(depth + 1, o->indent);
        pp_string(b, depth + 1, o, 1);

        skip_ws(b);
        if (advance(b) != ':') die_pos(b, "expected ':'");
        putchar(':');
        putchar(' ');

        pp_value(b, depth + 1, o, 0);

        skip_ws(b);
        char c = peek(b);
        if (c == '}') {
            b->pos++;
            putchar('\n');
            put_indent(depth, o->indent);
            put_color(o, CLR_BRACE);
            putchar('}');
            put_color(o, CLR_RESET);
            return;
        }
        if (c == ',') {
            b->pos++;
        } else {
            die_pos(b, "expected ',' or '}'");
        }
    }
}

static void pp_array(Buffer *b, int depth, const Options *o) {
    b->pos++; /* skip '[' */
    put_color(o, CLR_BRACE);
    putchar('[');
    put_color(o, CLR_RESET);

    skip_ws(b);
    if (peek(b) == ']') {
        b->pos++;
        put_color(o, CLR_BRACE);
        putchar(']');
        put_color(o, CLR_RESET);
        return;
    }

    putchar('\n');
    int first = 1;
    while (1) {
        if (!first) {
            putchar(',');
            putchar('\n');
        }
        first = 0;

        put_indent(depth + 1, o->indent);
        pp_value(b, depth + 1, o, 0);

        skip_ws(b);
        char c = peek(b);
        if (c == ']') {
            b->pos++;
            putchar('\n');
            put_indent(depth, o->indent);
            put_color(o, CLR_BRACE);
            putchar(']');
            put_color(o, CLR_RESET);
            return;
        }
        if (c == ',') {
            b->pos++;
        } else {
            die_pos(b, "expected ',' or ']'");
        }
    }
}

static void pp_value(Buffer *b, int depth, const Options *o, int is_key) {
    char c = peek(b);
    switch (c) {
        case '"': pp_string(b, depth, o, is_key); break;
        case '{': pp_object(b, depth, o);          break;
        case '[': pp_array(b, depth, o);            break;
        case 't': pp_literal(b, o, "true",  CLR_BOOL); break;
        case 'f': pp_literal(b, o, "false", CLR_BOOL); break;
        case 'n': pp_literal(b, o, "null",  CLR_NULL);  break;
        default:
            if (c == '-' || (c >= '0' && c <= '9'))
                pp_number(b, o);
            else
                die_pos(b, "unexpected character");
            break;
    }
}

/* ---------- main ---------- */

static void usage(void) {
    fprintf(stderr,
        "jpp %s - JSON Pretty Printer\n"
        "\n"
        "Usage:\n"
        "  jpp [options] [file]\n"
        "  cat file.json | jpp\n"
        "\n"
        "Options:\n"
        "  -t N   Indent width in spaces (default: %d)\n"
        "  -c     Force colored output\n"
        "  -C     Disable colored output\n"
        "  -h     Show this help\n",
        JPP_VERSION, DEFAULT_INDENT);
    exit(0);
}

int main(int argc, char **argv) {
    Options opts;
    opts.indent = DEFAULT_INDENT;
    opts.color  = -1; /* auto */

    const char *filename = NULL;
    int i = 1;
    while (i < argc) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                usage();
            } else if (strcmp(argv[i], "-c") == 0) {
                opts.color = 1; i++;
            } else if (strcmp(argv[i], "-C") == 0) {
                opts.color = 0; i++;
            } else if (strcmp(argv[i], "-t") == 0) {
                i++;
                if (i >= argc) die("option -t requires an argument");
                opts.indent = atoi(argv[i]);
                if (opts.indent < 0 || opts.indent > 16)
                    die("indent must be between 0 and 16");
                i++;
            } else {
                fprintf(stderr, "jpp: unknown option '%s'\n", argv[i]);
                exit(1);
            }
        } else {
            filename = argv[i];
            i++;
        }
    }

    /* auto-detect color */
    if (opts.color == -1)
        opts.color = isatty(STDOUT_FILENO);

    FILE *fp;
    if (filename) {
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "jpp: %s: %s\n", filename, strerror(errno));
            return 1;
        }
    } else {
        fp = stdin;
    }

    Buffer buf;
    buf.data = read_all(fp, &buf.len);
    buf.pos  = 0;
    if (fp != stdin) fclose(fp);

    skip_ws(&buf);
    if (buf.pos >= buf.len) die("empty input");

    pp_value(&buf, 0, &opts, 0);
    putchar('\n');

    /* check for trailing garbage */
    skip_ws(&buf);
    if (buf.pos < buf.len)
        die_pos(&buf, "unexpected data after JSON value");

    free(buf.data);
    return 0;
}
