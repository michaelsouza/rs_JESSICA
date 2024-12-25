import networkx as nx
from networkx.drawing.nx_pydot import read_dot

# Load the .dot file
dot_file_path = "/home/michael/gitrepos/rs_JESSICA/epanet-dev/call_tree.dot"
graph = read_dot(dot_file_path)

root = "BBSolver::epanet_solve(Epanet::Project&, int&, int&, bool, double&)"

# Get all descendants of the root
nodes = nx.descendants(graph, root)

# Include the root in the descendants
nodes.add(root)

# Get the subgraph of the descendants
graph = graph.subgraph(nodes).copy()

# Print all edges of the descendants
for edge in graph.edges():
    node1, node2 = edge
    # print(f"{node1.split('(')[0]} {node2.split('(')[0]}")

# The node is pass-through if it has only one outgoing edge and one incoming edge
count_node_out = {}
for edge in graph.edges():
    node1, node2 = edge
    if node1.startswith("0x"):
        count_node_out[node1] = count_node_out.get(node1, 0) + 1

node_pass_through = []
for node in count_node_out:
    if count_node_out[node] == 1:
        node_pass_through.append(node)

# create for each pass-through node, create an edge from the parent to the child and remove the pass-through node
for node in node_pass_through:
    parent = list(graph.predecessors(node))[0]
    child = list(graph.successors(node))[0]
    graph.add_edge(parent, child)
    graph.remove_node(node)

edges = []
for edge in graph.edges():
    node1, node2 = edge
    node1 = node1.split("(")[0]
    node2 = node2.split("(")[0]
    edges.append((node1, node2))

# Create a new graph with the edges
graph = nx.DiGraph()
graph.add_edges_from(edges)

print("=" * 20)
print("Edges:")
print("=" * 20)
message = []
for edge in graph.edges():
    message.append(f"{edge[0]} > {edge[1]}")

print("\n".join(sorted(message)))

# Create a dictionary of class names and their functions
class_functions = {}
for node in graph.nodes():
    tokens = node.rsplit("::", 1)
    class_name = tokens[0]
    if class_name not in class_functions:
        class_functions[class_name] = []
    if len(tokens) > 1:
        class_function = tokens[1]
        class_functions[class_name].append(node)


print(("=" * 20) + "\nClasses:\n" + ("=" * 20))
for class_name in sorted(class_functions):
    print(f"{class_name}")
    for function in class_functions[class_name]:
        print(f"    {function}")
