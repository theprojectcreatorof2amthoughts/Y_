#!/usr/bin/env bash
set -euo pipefail

actual="$(./y_ tests/smoke.y)"
expected="$(printf '10\nabab\n42')"

if [[ "$actual" != "$expected" ]]; then
    printf 'unexpected smoke-test output\nexpected:\n%s\nactual:\n%s\n' "$expected" "$actual" >&2
    exit 1
fi

if ./y_ tests/invalid_memory.y >/dev/null 2>&1; then
    printf 'out-of-bounds memory access unexpectedly succeeded\n' >&2
    exit 1
fi

printf 'Y_ smoke and error-path tests passed\n'