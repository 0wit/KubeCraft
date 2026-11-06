#!/usr/bin/env bash
# Breaks one source file on purpose, builds, runs one test, then ALWAYS restores the file and rebuilds clean.
# The editor must be closed (CLI build).
# Usage: bash mutate.sh <file relative to repo> '<sed expression>' <test name, e.g. KubeCraft.Integration.ChunkStreaming>
set -u
REPO="$(git rev-parse --show-toplevel)"
ENGINE="/c/Program Files/Epic Games/UE_5.8/Engine"
FILE="$REPO/$1"; EXPR="$2"; TEST="$3"
BACKUP="$(mktemp)"

build() {
	"$ENGINE/Build/BatchFiles/Build.bat" KubeCraftEditor Win64 Development \
		-Project="$REPO/KubeCraft.uproject" -WaitMutex -NoHotReloadFromIDE 2>&1 | grep -E " error |Result:"
}
MUTATED=0
restore() {
	cp "$BACKUP" "$FILE" && rm -f "$BACKUP"
	if [ "$MUTATED" = 1 ]; then
		echo "== Restored $FILE - rebuilding clean"
		build
	fi
}

if tasklist //FI "IMAGENAME eq UnrealEditor.exe" 2>/dev/null | grep -qi UnrealEditor; then
	echo "Close the Unreal editor first (CLI build)." >&2; exit 1
fi

cp "$FILE" "$BACKUP"
trap restore EXIT
sed -i "$EXPR" "$FILE"
if cmp -s "$FILE" "$BACKUP"; then
	echo "The sed expression changed nothing - check it (CRLF files: don't anchor on \$)." >&2; exit 1
fi
MUTATED=1
echo "== Mutation:"; diff "$BACKUP" "$FILE"

echo "== Build"; build
echo "== Test $TEST (should FAIL)"
"$ENGINE/Binaries/Win64/UnrealEditor-Cmd.exe" "$REPO/KubeCraft.uproject" \
	-ExecCmds="Automation RunTests $TEST; Quit" -unattended -nopause -nullrhi -nosplash -log -stdout 2>&1 \
	| grep -E "Test Completed|LogAutomationController: Error" | head -10
