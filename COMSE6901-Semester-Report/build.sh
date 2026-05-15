#!/bin/bash
set -e

# latexminted is incompatible with Python 3.14's argparse; force 3.13
# by creating a temp dir with a python3 symlink pointing to 3.13
_PY_SHIM=$(mktemp -d)
ln -s /opt/homebrew/bin/python3.13 "$_PY_SHIM/python3"
export PATH="$_PY_SHIM:$PATH"
trap 'rm -rf "$_PY_SHIM"' EXIT

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$REPO_DIR/build"

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

latexmk -lualatex -shell-escape \
  -output-directory="$OUT_DIR" \
  "$REPO_DIR/main.tex"

echo "Build complete. Output in: $OUT_DIR"
open "$OUT_DIR/main.pdf"
