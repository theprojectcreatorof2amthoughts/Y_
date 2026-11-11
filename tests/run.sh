#!/usr/bin/env bash
set -euo pipefail

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

./y_ examples/modern.yc > "$tmp"

grep -Fq "excellent" "$tmp"
grep -Fq "Floor division: 3" "$tmp"
grep -Fq "Square root: 9" "$tmp"
grep -Fq "String length: 11" "$tmp"
grep -Fq "Radius type: number" "$tmp"
grep -Fq "Rounded area:" "$tmp"

printf '%s\n' "Y_ modern syntax smoke tests passed."
