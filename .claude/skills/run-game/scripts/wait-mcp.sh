#!/usr/bin/env bash
# Waits until the Unreal MCP server (started by the editor) answers, up to ~3 minutes.
# Usage: bash wait-mcp.sh
for i in $(seq 1 60); do
	if curl -s -o /dev/null -m 2 http://127.0.0.1:8000/mcp; then
		echo "MCP up after $((i * 3)) s"
		exit 0
	fi
	sleep 3
done
echo "MCP did not answer - is the editor starting? Is Auto Start Server on?" >&2
exit 1
