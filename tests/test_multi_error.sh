#!/bin/sh
set -eu

check_errors() {
    mode="$1"
    output="$(mktemp)"
    binary="$(mktemp)"
    rm -f "$binary"

    if [ "$mode" = "--check" ]; then
        if ./gyy --check tests/multiple_errors.yc > /dev/null 2>"$output"; then
            echo "FAIL: gyy --check accepted a file with syntax errors"
            rm -f "$output" "$binary"
            exit 1
        fi
    else
        if ./gyy tests/multiple_errors.yc -o "$binary" > /dev/null 2>"$output"; then
            echo "FAIL: normal gyy compilation accepted a file with syntax errors"
            rm -f "$output" "$binary"
            exit 1
        fi
        if [ -e "$binary" ]; then
            echo "FAIL: gyy left an executable behind after syntax errors"
            rm -f "$output" "$binary"
            exit 1
        fi
    fi

    count="$(grep -c ': error:' "$output" || true)"
    cat "$output"
    rm -f "$output" "$binary"

    if [ "$count" -lt 2 ]; then
        echo "FAIL: $mode expected at least two diagnostics, received $count"
        exit 1
    fi

    echo "PASS: $mode reported $count independent errors"
}

check_errors "--check"
check_errors "compile"
