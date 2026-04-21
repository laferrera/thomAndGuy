#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
JUCER_FILE="$PROJECT_DIR/ThomAndGuy.jucer"
PROJUCER="$HOME/JUCE/Projucer.app/Contents/MacOS/Projucer"
XCODE_PROJECT="$PROJECT_DIR/Builds/MacOSX/ThomAndGuy.xcodeproj"

if [ "$1" = "test" ]; then
    echo "==> Building..."
    BUILDING_FOR_TEST=1 "$0"
    echo "==> Running tests..."
    ./Builds/MacOSX/build/Debug/ThomAndGuy.app/Contents/MacOS/ThomAndGuy --run-tests
    exit $?
fi

CONFIG="${1:-Debug}"

pkill -x ThomAndGuy 2>/dev/null || true

echo "==> Resaving Projucer project..."
"$PROJUCER" --resave "$JUCER_FILE"

echo "==> Building ($CONFIG)..."
xcodebuild -project "$XCODE_PROJECT" \
           -configuration "$CONFIG" \
           -target "ThomAndGuy - Standalone Plugin" \
           -quiet

echo "==> Done."

if [ -z "$BUILDING_FOR_TEST" ]; then
    open ./Builds/MacOSX/build/Debug/ThomAndGuy.app
fi
