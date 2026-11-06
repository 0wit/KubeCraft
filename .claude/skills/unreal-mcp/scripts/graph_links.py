# Paste as the body of ProgrammaticToolset.execute_tool_script (set PATH and GRAPHS first).
# Prints every node of each graph with what feeds its input pins - the real links, not read_graph_dsl's names.
import json
BT = 'editor_toolset.toolsets.blueprint.BlueprintTools.'
PATH = '/Game/KubeCraft/UI/WBP_X.WBP_X'
GRAPHS = ['EventGraph']

def summary(graph):
    nodes = execute_tool(BT + 'find_nodes', json.dumps({'graph': {'refPath': PATH + ':' + graph}, 'title': ''}))['returnValue']
    infos = execute_tool(BT + 'get_node_infos', json.dumps({'nodes': nodes}))['returnValue']
    lines = []
    for info in infos:
        ins = []
        for pin in info['input_pins']:
            sources = [c['node']['refPath'].split('.')[-1] + '#' + str(c['index_id']) for c in pin['connected_pins']]
            ins.append(pin['name'] + '<-' + (','.join(sources) if sources else repr(pin['value'])))
        lines.append(info['node']['refPath'].split('.')[-1] + ' ' + info['type_id'] + ' ' + '; '.join(ins))
    return lines

def run():
    return {g: summary(g) for g in GRAPHS}
