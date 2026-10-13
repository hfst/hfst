#!/bin/sh
#
# Generate the flex/bison parser and lexer sources that libhfst needs.
#
# In an autotools build these are produced by `make` in
# libhfst/src/parsers/. For the standalone (pyproject/cibuildwheel) build we
# reproduce them here with plain `bison`/`flex`, emitting the .cc/.hh files that
# setup.py compiles. This is a portable adaptation of
# libhfst/src/parsers/compile-parsers-win.sh.
#
# Run from libhfst/src/parsers/ (setup.py invokes it with that cwd), or pass the
# parsers directory as the first argument.

set -e

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "Usage: generate-parsers.sh [PARSERS_DIR]"
    echo "Generate flex/bison sources for libhfst (needs bison and flex)."
    exit 0
fi

if [ -n "$1" ]; then
    cd "$1"
fi

BISON="${BISON:-bison}"
FLEX="${FLEX:-flex}"

# Parsers (bison). --defines writes the token header the matching lexer needs.
for base in xre_parse pmatch_parse lexc-parser xfst-parser sfst-compiler \
            htwolcpre1-parser htwolcpre2-parser htwolcpre3-parser; do
    echo "bison: ${base}.yy -> ${base}.cc"
    "$BISON" --defines="${base}.hh" --output="${base}.cc" "${base}.yy"
done

# Lexers (flex).
for pair in \
    "xre_lex.ll:xre_lex.cc" \
    "pmatch_lex.ll:pmatch_lex.cc" \
    "lexc-lexer.ll:lexc-lexer.cc" \
    "xfst-lexer.ll:xfst-lexer.cc" \
    "sfst-scanner.ll:sfst-scanner.cc" \
    "htwolcpre1-lexer.ll:htwolcpre1-lexer.cc" \
    "htwolcpre2-lexer.ll:htwolcpre2-lexer.cc" \
    "htwolcpre3-lexer.ll:htwolcpre3-lexer.cc"; do
    src="${pair%%:*}"
    dst="${pair##*:}"
    echo "flex: ${src} -> ${dst}"
    "$FLEX" --outfile="${dst}" "${src}"
done

echo "Parser/lexer sources generated."
