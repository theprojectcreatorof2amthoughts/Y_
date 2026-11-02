#!/usr/bin/env bash
set -euo pipefail

actual="$(./y_ tests/smoke.y)"
expected="$(printf '10\nabab\n42\nyes\nescaped\nvoid\nsymbol-name-ok\nescaped-symbol-name-ok\ndeclaration-check-ok')"

if [[ "$actual" != "$expected" ]]; then
    printf 'unexpected smoke-test output\nexpected:\n%s\nactual:\n%s\n' "$expected" "$actual" >&2
    exit 1
fi

if ./y_ tests/invalid_memory.y >/dev/null 2>&1; then
    printf 'out-of-bounds memory access unexpectedly succeeded\n' >&2
    exit 1
fi

error_output_file="$(mktemp)"
trap 'rm -f "$error_output_file"' EXIT
normal_output="$(./y_ tests/error_output.y 2>"$error_output_file")"
error_output="$(<"$error_output_file")"

if [[ "$normal_output" != "normal message" || "$error_output" != "diagnostic message" ]]; then
    printf 'errstate output did not use stderr exclusively\nstdout: %s\nstderr: %s\n' "$normal_output" "$error_output" >&2
    exit 1
fi

parameter_errors="$(./y_ tests/input_args.y 2>"$error_output_file")"
expected_parameter_errors="$(printf 'Input has more characters!\nOutput has more characters!\nAn error has occured in <input_lib>')"
actual_parameter_errors="$(<"$error_output_file")"
if [[ -n "$parameter_errors" || "$actual_parameter_errors" != "$expected_parameter_errors" ]]; then
    printf 'script parameters or errstate output did not behave as expected\nstdout: %s\nstderr: %s\n' "$parameter_errors" "$actual_parameter_errors" >&2
    exit 1
fi

printf 'Y_ smoke and error-path tests passed\n'