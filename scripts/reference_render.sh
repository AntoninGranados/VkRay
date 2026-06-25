#!/bin/bash
# Usage: reference_render.sh <vkRayReference_binary> <project_root>
# Runs a reference render when the VERSION file has changed since the last render.

BINARY="$1"
PROJECT_ROOT="$2"

VERSION_FILE="$PROJECT_ROOT/VERSION"
if [ ! -f "$VERSION_FILE" ]; then
    echo "[reference] No VERSION file found, skipping."
    exit 0
fi

VERSION=$(cat "$VERSION_FILE" | tr -d '[:space:]')

OUTPUT_DIR="$PROJECT_ROOT/outputs/reference"
LAST_VERSION_FILE="$OUTPUT_DIR/.last_version"
mkdir -p "$OUTPUT_DIR"

if [ -f "$LAST_VERSION_FILE" ] && [ "$(cat "$LAST_VERSION_FILE")" = "$VERSION" ]; then
    echo "[reference] No changes since last render (v${VERSION}), skipping."
    exit 0
fi

OUTPUT_PATH="$OUTPUT_DIR/ref_v${VERSION}.png"
echo "[reference] New version v${VERSION}, rendering to $OUTPUT_PATH ..."

"$BINARY" --reference "$OUTPUT_PATH"
STATUS=$?

if [ $STATUS -eq 0 ]; then
    echo "$VERSION" > "$LAST_VERSION_FILE"
    echo "[reference] Done."
else
    echo "[reference] Render failed (exit $STATUS)."
fi
