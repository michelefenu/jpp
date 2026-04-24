# JPP - JSON Pretty Printer

A minimal, dependency-free JSON pretty printer for the terminal. Written in **C99** using only `libc`. No external libraries required.

## Features

- Colored output with auto-detection (on for TTY, off for pipes/files)
- Configurable indentation width
- Reads from stdin or from a file argument
- Helpful error messages with line and column numbers
- Handles all JSON types: objects, arrays, strings (with escapes), numbers, booleans, and null
- Single file, zero dependencies, builds in under a second

## Building

make

Or manually:

gcc -std=c99 -O2 -Wall -Wextra -pedantic -o jpp jpp.c

## Installing

sudo make install

This installs `jpp` to `/usr/local/bin`. To change the location:

make install PREFIX=/opt

## Uninstalling

sudo make uninstall

## Usage

**Pipe from stdin:**

cat data.json | jpp
curl -s https://api.example.com | jpp
echo '{"a":1,"b":[2,3]}' | jpp

**Pass a file directly:**

jpp data.json

**Write formatted output to a file (color auto-disabled):**

jpp data.json > pretty.json

## Options

| Option | Description |
|--------|-------------|
| `-t N` | Set indent width to N spaces (default: 2, max: 16) |
| `-c` | Force colored output (even when piping to a file) |
| `-C` | Disable colored output (even on a TTY) |
| `-h` | Show help message |

## Examples

**Input:**

{"name":"jpp","version":1,"tags":["json","pretty","c99"],"fast":true}

**Output (`jpp -t 2`):**

{
  "name": "jpp",
  "version": 1,
  "tags": [
    "json",
    "pretty",
    "c99"
  ],
  "fast": true
}

**With 4-space indent:**

jpp -t 4 data.json

**Force no color for copying into docs:**

jpp -C < data.json

## Error Handling

`jpp` reports errors with line and column numbers:

$ echo '{"a": }' | jpp
jpp: error at line 1, col 7: unexpected character

$ echo '{"a": "hello' | jpp
jpp: error at line 1, col 13: unterminated string

## License

**BSD-2-Clause** license

