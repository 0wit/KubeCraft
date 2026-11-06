#!/usr/bin/env bash
# Checks docs/*.md: aligned tables, links to headings that exist, CRLF line endings, no emoji.
# Prints one line per problem and exits 1; prints "OK" and exits 0 when all is good.
# Usage: bash check-docs.sh [docs folder]   (default: the repo's docs/)
export LC_ALL=C.UTF-8
cd "${1:-$(git rev-parse --show-toplevel)/docs}" || exit 1

# GitHub anchor of every heading, as "file#anchor" (lower case, punctuation dropped, spaces -> "-").
anchors="$(for f in *.md; do
	awk -v F="$f" '{ sub(/\r$/, "") } /^#+ / {
		h = $0; sub(/^#+ /, "", h); h = tolower(h)
		gsub(/[^a-z0-9 _-]/, "", h); gsub(/ /, "-", h)
		print F "#" h }' "$f"
done)"

problems="$(for f in *.md; do
	# 1. Tables: every row as wide (in characters) as the row above it.
	awk -v F="$f" '{ sub(/\r$/, "") }
		/^\|/ { n = length($0); if (prev && n != prev) print F ":" NR ": table row is " n " wide, row above " prev; prev = n; next }
		{ prev = 0 }' "$f"

	# 2. Links to headings: [text](#anchor) or [text](OTHER.md#anchor).
	tr -d '\r' < "$f" | grep -o '](\([A-Za-z]*\.md\)\?#[^)]*)' | sed 's/^](//; s/)$//' | sort -u | while read -r link; do
		target="${link%%#*}"; [ -z "$target" ] && target="$f"
		grep -qxF "$target#${link#*#}" <<< "$anchors" || echo "$f: link to a missing heading: $link"
	done

	# 3. Line endings: the working copy uses CRLF (git stores LF).
	file "$f" | grep -q CRLF || echo "$f: not CRLF - convert: sed -i 's/\r\$//; s/\$/\r/' docs/$f"

	# 4. No emoji status markers.
	grep -nP '[\x{1F300}-\x{1FAFF}\x{2705}\x{274C}\x{26A0}]' "$f" | sed "s/^/$f:/; s/\$/  <- emoji/"
done)"

if [ -n "$problems" ]; then
	echo "$problems"
	exit 1
fi
echo "OK"
