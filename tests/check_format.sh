#!/bin/bash
set -e

GIT_CLANG_FORMAT="$(command -v git-clang-format || true)"
CLANG_FORMAT="$(command -v clang-format || true)"
if [ -z "$GIT_CLANG_FORMAT" ] && [ -x /opt/homebrew/opt/llvm/bin/git-clang-format ]; then
    GIT_CLANG_FORMAT=/opt/homebrew/opt/llvm/bin/git-clang-format
    CLANG_FORMAT=/opt/homebrew/opt/llvm/bin/clang-format
fi

if [ -z "$GIT_CLANG_FORMAT" ]; then
    echo "clang-format/git-clang-format not found, skipping formatting check."
    exit 0
fi

FILES=$(git diff --cached --name-only --diff-filter=ACMR -- '*.cpp' '*.hpp' '*.cc' '*.h' '*.json' | grep -v '^external/' | grep -v '^build/' || true)
if [ -z "$FILES" ]; then
    exit 0
fi

FORMAT_OUTPUT=$("$GIT_CLANG_FORMAT" --binary "$CLANG_FORMAT" --staged --diff -- $FILES) || true
if echo "$FORMAT_OUTPUT" | grep -q "^diff --git"; then
    echo "$FORMAT_OUTPUT"
    echo "Formatting check failed. Run: git-clang-format --staged to fix, then re-stage."
    exit 1
fi
