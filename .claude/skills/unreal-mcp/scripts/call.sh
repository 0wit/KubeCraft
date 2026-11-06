#!/usr/bin/env bash
# Usage: call.sh <toolset> <tool> [<args-json> | @<file with args json>]
# Calls an Unreal MCP toolset tool over HTTP (toolset "" = top-level tool) and prints the result text.
# The session id is cached in $MCP_STATE_DIR/mcp.sid - delete it after an editor restart.
DIR="${MCP_STATE_DIR:-${TMPDIR:-/tmp}}"; URL=http://127.0.0.1:8000/mcp
H=(-H "Content-Type: application/json" -H "Accept: application/json, text/event-stream")
if [ ! -s "$DIR/mcp.sid" ]; then
	curl -s -D "$DIR/mcp.hdr" -o /dev/null -m 30 "${H[@]}" "$URL" \
		-d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"cli","version":"1"}}}'
	tr -d '\r' < "$DIR/mcp.hdr" | grep -i '^mcp-session-id:' | awk '{print $2}' > "$DIR/mcp.sid"
fi
ARGS="${3:-}"; [ -z "$ARGS" ] && ARGS='{}'
node -e '
const fs = require("fs");
let [toolset, tool, args] = process.argv.slice(1);
if (args.startsWith("@")) args = fs.readFileSync(args.slice(1), "utf8");
const p = { tool_name: tool, arguments: JSON.parse(args) };
if (toolset) p.toolset_name = toolset;
process.stdout.write(JSON.stringify({ jsonrpc: "2.0", id: 2, method: "tools/call", params: { name: "call_tool", arguments: p } }));
' "$1" "$2" "$ARGS" > "$DIR/mcp.body" || exit 1
curl -s -m 600 "${H[@]}" -H "Mcp-Session-Id: $(cat "$DIR/mcp.sid")" "$URL" --data-binary "@$DIR/mcp.body" | node -e '
let s = "";
process.stdin.on("data", d => s += d).on("end", () => {
	try {
		const j = JSON.parse(s.replace(/^data: /m, ""));
		if (j.error) { console.log("ERROR", JSON.stringify(j.error)); return; }
		for (const c of j.result.content || []) console.log(c.text);
		if (j.result.isError) console.log("(isError)");
	} catch (e) { console.log(s); }
});'
