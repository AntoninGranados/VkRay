#!/bin/bash
set -e

VKRAY="$1"
GENERATED=$(mktemp -d)
trap 'rm -rf "$GENERATED"' EXIT

"$VKRAY" --generate-docs "$GENERATED" > /dev/null

STATUS=0
for DOC in parameters.md components.md; do
    if ! diff -u "docs/$DOC" "$GENERATED/$DOC"; then
        STATUS=1
    fi
done

if [ "$STATUS" -ne 0 ]; then
    echo "Generated docs are out of date. Run: $VKRAY --generate-docs docs, then re-stage."
fi
exit "$STATUS"
