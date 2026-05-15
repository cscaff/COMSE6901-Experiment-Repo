#!/usr/bin/env bash
# lean_check.sh — compile and verify a Lean 4 snippet or file.
#
# Automatically uses Mathlib (via Lake) when the code contains import statements.
# Otherwise runs lean directly for fast standalone checks.
#
# Usage:
#   lean_check.sh FILE.lean              # check a file
#   lean_check.sh -c 'import Mathlib\ntheorem ...'  # inline code
#   echo 'theorem t : 1=1 := rfl' | lean_check.sh   # stdin
#
# Exit codes: 0 = success (no errors, no sorry), 1 = errors/sorry, 2 = usage error
#
# Output (agent-readable):
#   STATUS: PASS | FAIL | FAIL (sorry)
#   ERRORS: <n>
#   WARNINGS: <n>
#   MODE: standalone | mathlib
#   --- lean output ---
#   <raw lean output>

set -euo pipefail

LEAN="${LEAN_BIN:-lean}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MATHLIB_PROJECT="$SCRIPT_DIR/mathlib_project"
TMPFILE=""

cleanup() {
  [[ -n "$TMPFILE" && -f "$TMPFILE" ]] && rm -f "$TMPFILE"
}
trap cleanup EXIT

usage() {
  echo "Usage: lean_check.sh [-c CODE] [FILE]" >&2
  echo "  -c CODE   inline Lean code string" >&2
  echo "  FILE      path to a .lean file" >&2
  echo "  (no args) reads Lean code from stdin" >&2
  exit 2
}

# Parse args
INLINE_CODE=""
TARGET_FILE=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -c)
      shift
      [[ $# -eq 0 ]] && { echo "lean_check.sh: -c requires an argument" >&2; usage; }
      INLINE_CODE="$1"
      shift
      ;;
    -h|--help) usage ;;
    -*)
      echo "lean_check.sh: unknown option $1" >&2
      usage
      ;;
    *)
      [[ -n "$TARGET_FILE" ]] && { echo "lean_check.sh: only one file argument allowed" >&2; usage; }
      TARGET_FILE="$1"
      shift
      ;;
  esac
done

# Resolve input to a file lean will read
if [[ -n "$INLINE_CODE" ]]; then
  TMPFILE="$(mktemp /tmp/lean_check_XXXXXX.lean)"
  printf '%s\n' "$INLINE_CODE" > "$TMPFILE"
  TARGET_FILE="$TMPFILE"
elif [[ -n "$TARGET_FILE" ]]; then
  [[ -f "$TARGET_FILE" ]] || { echo "lean_check.sh: file not found: $TARGET_FILE" >&2; exit 2; }
elif [[ ! -t 0 ]]; then
  TMPFILE="$(mktemp /tmp/lean_check_XXXXXX.lean)"
  cat > "$TMPFILE"
  TARGET_FILE="$TMPFILE"
else
  usage
fi

# Decide mode: use Lake+Mathlib if any import lines are present
if grep -qE '^import ' "$TARGET_FILE"; then
  MODE="mathlib"
  if [[ ! -d "$MATHLIB_PROJECT" ]]; then
    echo "lean_check.sh: mathlib_project not found at $MATHLIB_PROJECT" >&2
    exit 2
  fi
  LEAN_OUTPUT="$(cd "$MATHLIB_PROJECT" && lake env "$LEAN" "$TARGET_FILE" 2>&1)" || true
else
  MODE="standalone"
  LEAN_OUTPUT="$("$LEAN" "$TARGET_FILE" 2>&1)" || true
fi

# Parse results
ERROR_COUNT=$(echo "$LEAN_OUTPUT"   | grep -c '^.*: error:'   || true)
WARNING_COUNT=$(echo "$LEAN_OUTPUT" | grep -c '^.*: warning:' || true)
SORRY_COUNT=$(echo "$LEAN_OUTPUT"   | grep -c 'declaration uses `sorry`\|uses sorry' || true)

if [[ "$ERROR_COUNT" -gt 0 ]]; then
  STATUS="FAIL"
  EXIT_CODE=1
elif [[ "$SORRY_COUNT" -gt 0 ]]; then
  STATUS="FAIL (sorry)"
  EXIT_CODE=1
else
  STATUS="PASS"
  EXIT_CODE=0
fi

echo "STATUS: $STATUS"
echo "ERRORS: $ERROR_COUNT"
echo "WARNINGS: $WARNING_COUNT"
echo "MODE: $MODE"
echo "--- lean output ---"
if [[ -z "$LEAN_OUTPUT" ]]; then
  echo "(no output)"
else
  echo "$LEAN_OUTPUT"
fi

exit "$EXIT_CODE"
