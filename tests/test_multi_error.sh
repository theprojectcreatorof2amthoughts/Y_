#!/bin/sh
set -eu

output="$(mktemp)"
if ./gyy --check tests/multiple_errors.yc > /dev/null 2>"$output"; then
    echo "FAIL: gyy accepted a file with syntax errors"
    rm -f "$output"
    exit 1
fi

cat "$output"
count="$(grep -c ': error:' "$output" || true)"
rm -f "$output"

if [ "$count" -lt 2 ]; then
    echo "FAIL: expected at least two diagnostics, received $count"
    exit 1
fi

echo "PASS: gyy reported $count independent errors"
