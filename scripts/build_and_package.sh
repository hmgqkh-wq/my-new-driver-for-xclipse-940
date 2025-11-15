#!/usr/bin/env bash
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
STAGE="$BUILD/stage"
rm -rf "$STAGE"
mkdir -p "$STAGE/usr/lib" "$STAGE/usr/etc/exynosfull" "$STAGE/usr/share/doc/exynosfull"
# find compiled library
find "$BUILD" -type f -name "libexynosfull*.so" -exec cp {} "$STAGE/usr/lib/libexynosfull.so" \; || true
cp -r "$ROOT/etc" "$STAGE/usr/" || true
cp -r "$ROOT/profiles" "$STAGE/usr/etc/" || true
cp "$ROOT/README.md" "$STAGE/usr/share/doc/"
cp "$ROOT/LICENSE" "$STAGE/usr/share/doc/"
OUT="$ROOT/exynosx940-fullport-android-arm64.tar.gz"
tar -C "$STAGE" -czf "$OUT" .
echo "Created $OUT"
