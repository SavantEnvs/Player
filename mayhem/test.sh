#!/usr/bin/env bash
# mayhem/test.sh — run upstream's doctest suite (built by mayhem/build.sh) and report CTRF counts.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

RUNNER=mayhem/build-tests/test_runner
if [ ! -x "$RUNNER" ]; then
  echo "test.sh: $RUNNER missing — mayhem/build.sh must build it" >&2
  emit_ctrf doctest 0 1
  exit 1
fi

out=$("$RUNNER" --no-colors 2>&1); rc=$?
printf '%s\n' "$out" | tail -n 15
# doctest summary: "[doctest] test cases:  N |  P passed | F failed | S skipped"
line=$(printf '%s\n' "$out" | grep -E '^\[doctest\] test cases:' | tail -n 1)
if [ -z "$line" ]; then
  echo "test.sh: no doctest summary (rc=$rc)" >&2
  emit_ctrf doctest 0 1
  exit 1
fi
passed=$(sed -E 's/.*\| *([0-9]+) passed.*/\1/' <<<"$line")
failed=$(sed -E 's/.*\| *([0-9]+) failed.*/\1/' <<<"$line")
skipped=$(sed -E 's/.*\| *([0-9]+) skipped.*/\1/' <<<"$line")
[ "$rc" -ne 0 ] && [ "$failed" -eq 0 ] && failed=1
emit_ctrf doctest "$passed" "$failed" "$skipped"
