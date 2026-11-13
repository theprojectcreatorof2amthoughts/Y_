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

check_undefined_names() {
    mode="$1"
    output="$(mktemp)"
    binary="$(mktemp)"
    rm -f "$binary"

    if [ "$mode" = "--check" ]; then
        if ./gyy --check tests/undefined_names.yc > /dev/null 2>"$output"; then
            echo "FAIL: --check accepted undefined names"
            rm -f "$output" "$binary"
            exit 1
        fi
    else
        if ./gyy tests/undefined_names.yc -o "$binary" > /dev/null 2>"$output"; then
            echo "FAIL: compiler accepted undefined names"
            rm -f "$output" "$binary"
            exit 1
        fi
        if [ -e "$binary" ]; then
            echo "FAIL: compiler created an executable despite undefined names"
            rm -f "$output" "$binary"
            exit 1
        fi
    fi

    count="$(grep -c ': error:' "$output" || true)"
    if ! grep -q "missing_global" "$output" || ! grep -q "missing_inside_function" "$output"; then
        cat "$output"
        echo "FAIL: expected diagnostics for both undefined names"
        rm -f "$output" "$binary"
        exit 1
    fi
    cat "$output"
    rm -f "$output" "$binary"
    if [ "$count" -lt 2 ]; then
        echo "FAIL: expected at least two semantic diagnostics, received $count"
        exit 1
    fi
    echo "PASS: $mode rejected undefined names before execution"
}

check_undefined_names "--check"
check_undefined_names "compile"
