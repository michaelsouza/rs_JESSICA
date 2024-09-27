import networkx as nx
import matplotlib.pyplot as plt
import re
from matplotlib.lines import Line2D


def parse_inp_file(file_path):
    """
    Parses the EPANET .inp file and extracts nodes, links, and their coordinates.

    Returns:
        nodes: Dictionary with node IDs as keys and attributes as values.
        links: List of dictionaries containing link information.
    """
    nodes = {}
    links = []
    coordinates = {}
    vertices = {}

    current_section = None
    with open(file_path, "r", encoding="utf-8") as file:
        for line in file:
            # Remove comments and trim whitespace
            line = line.split(";")[0].strip()
            if not line:
                continue
            # Check for section headers
            section_match = re.match(r"\[(.+)\]", line, re.IGNORECASE)
            if section_match:
                current_section = section_match.group(1).upper()
                continue
            # Parse sections
            if current_section == "JUNCTIONS":
                # Expecting: ID, Elev, Demand, Pattern
                parts = re.split(r"\s+", line)
                if len(parts) < 4:
                    continue
                node_id = parts[0]
                nodes[node_id] = {
                    "type": "Junction",
                    "elevation": float(parts[1]),
                    "demand": float(parts[2]),
                    "pattern": parts[3],
                }
            elif current_section == "RESERVOIRS":
                # Expecting: ID, Head, Pattern
                parts = re.split(r"\s+", line)
                if len(parts) < 2:
                    continue
                node_id = parts[0]
                head = float(parts[1])
                pattern = parts[2] if len(parts) >= 3 else None
                nodes[node_id] = {"type": "Reservoir", "head": head, "pattern": pattern}
            elif current_section == "TANKS":
                # Expecting: ID, Elevation, InitLevel, MinLevel, MaxLevel, Diameter, MinVol, VolCurve
                parts = re.split(r"\s+", line)
                if len(parts) < 7:
                    continue
                node_id = parts[0]
                nodes[node_id] = {
                    "type": "Tank",
                    "elevation": float(parts[1]),
                    "init_level": float(parts[2]),
                    "min_level": float(parts[3]),
                    "max_level": float(parts[4]),
                    "diameter": float(parts[5]),
                    "min_vol": float(parts[6]),
                    "vol_curve": parts[7] if len(parts) >= 8 else None,
                }
            elif current_section == "PIPES":
                # Expecting: ID, Node1, Node2, Length, Diameter, Roughness, MinorLoss, Status
                parts = re.split(r"\s+", line)
                if len(parts) < 8:
                    continue
                link_id = parts[0]
                node1 = parts[1]
                node2 = parts[2]
                links.append(
                    {
                        "id": link_id,
                        "type": "Pipe",
                        "node1": node1,
                        "node2": node2,
                        "length": float(parts[3]),
                        "diameter": float(parts[4]),
                        "roughness": float(parts[5]),
                        "minor_loss": float(parts[6]),
                        "status": parts[7],
                    }
                )
            elif current_section == "PUMPS":
                # Expecting: ID, Node1, Node2, Parameters
                parts = re.split(r"\s+", line)
                if len(parts) < 4:
                    continue
                link_id = parts[0]
                node1 = parts[1]
                node2 = parts[2]
                # Additional parameters are concatenated
                parameters = " ".join(parts[3:])
                links.append(
                    {
                        "id": link_id,
                        "type": "Pump",
                        "node1": node1,
                        "node2": node2,
                        "parameters": parameters,
                    }
                )
            elif current_section == "VALVES":
                # Expecting: ID, Node1, Node2, Diameter, Type, Setting, MinorLoss
                parts = re.split(r"\s+", line)
                if len(parts) < 7:
                    continue
                link_id = parts[0]
                node1 = parts[1]
                node2 = parts[2]
                diameter = float(parts[3])
                valve_type = parts[4]
                setting = parts[5]
                minor_loss = float(parts[6])
                links.append(
                    {
                        "id": link_id,
                        "type": "Valve",
                        "node1": node1,
                        "node2": node2,
                        "diameter": diameter,
                        "valve_type": valve_type,
                        "setting": setting,
                        "minor_loss": minor_loss,
                    }
                )
            elif current_section == "COORDINATES":
                # Expecting: Node, X-Coord, Y-Coord
                parts = re.split(r"\s+", line)
                if len(parts) < 3:
                    continue
                node_id = parts[0]
                x = float(parts[1])
                y = float(parts[2])
                coordinates[node_id] = (x, y)
            elif current_section == "VERTICES":
                # Expecting: Link, X-Coord, Y-Coord
                parts = re.split(r"\s+", line)
                if len(parts) < 3:
                    continue
                link_id = parts[0]
                x = float(parts[1])
                y = float(parts[2])
                if link_id not in vertices:
                    vertices[link_id] = []
                vertices[link_id].append((x, y))

    # Assign coordinates to nodes
    for node_id, coord in coordinates.items():
        if node_id in nodes:
            nodes[node_id]["coord"] = coord

    # Attach vertices to links
    for link in links:
        link_id = link["id"]
        if link_id in vertices:
            link["vertices"] = vertices[link_id]

    return nodes, links


def build_graph(nodes, links):
    """
    Builds a NetworkX graph from nodes and links.

    Returns:
        G: NetworkX graph with nodes and edges added.
    """
    G = nx.Graph()

    # Add nodes with attributes
    for node_id, attrs in nodes.items():
        G.add_node(node_id, **attrs)

    # Add edges with attributes
    for link in links:
        node1 = link["node1"]
        node2 = link["node2"]
        if node1 not in G.nodes or node2 not in G.nodes:
            continue  # Skip if nodes are not defined
        edge_attrs = link.copy()
        # Remove 'id', 'node1', 'node2' from attributes
        edge_attrs.pop("id", None)
        edge_attrs.pop("node1", None)
        edge_attrs.pop("node2", None)
        G.add_edge(node1, node2, **edge_attrs)

    return G


def visualize_network(G):
    """
    Visualizes the NetworkX graph with different symbols and colors for node and link types.
    """
    pos = {}
    node_shapes = {"Junction": "o", "Reservoir": "s", "Tank": "D"}
    node_color_map = {"Junction": "blue", "Reservoir": "green", "Tank": "orange"}

    # Assign positions
    for node, data in G.nodes(data=True):
        if "coord" in data:
            pos[node] = data["coord"]
        else:
            pos[node] = (0, 0)  # Default position if missing

    # Draw nodes with different shapes and colors
    for node_type, shape in node_shapes.items():
        nodelist = [
            node for node, attr in G.nodes(data=True) if attr.get("type") == node_type
        ]
        nx.draw_networkx_nodes(
            G,
            pos,
            nodelist=nodelist,
            node_shape=shape,
            node_color=node_color_map.get(node_type, "grey"),
            node_size=500,
            label=node_type,
        )

    # Draw edges with different styles based on link type
    pipe_edges = [
        (u, v) for u, v, attr in G.edges(data=True) if attr.get("type") == "Pipe"
    ]
    pump_edges = [
        (u, v) for u, v, attr in G.edges(data=True) if attr.get("type") == "Pump"
    ]
    valve_edges = [
        (u, v) for u, v, attr in G.edges(data=True) if attr.get("type") == "Valve"
    ]

    # Draw pipes
    nx.draw_networkx_edges(
        G, pos, edgelist=pipe_edges, edge_color="black", width=2, label="Pipe"
    )

    # Draw pumps
    nx.draw_networkx_edges(
        G,
        pos,
        edgelist=pump_edges,
        edge_color="red",
        width=2,
        style="dashed",
        label="Pump",
    )

    # Draw valves
    nx.draw_networkx_edges(
        G,
        pos,
        edgelist=valve_edges,
        edge_color="purple",
        width=2,
        style="dotted",
        label="Valve",
    )

    # Draw labels
    nx.draw_networkx_labels(G, pos, font_size=8, font_family="sans-serif")

    # Create custom legend
    legend_elements = [
        Line2D(
            [0],
            [0],
            marker="o",
            color="w",
            label="Junction",
            markerfacecolor="blue",
            markersize=10,
        ),
        Line2D(
            [0],
            [0],
            marker="s",
            color="w",
            label="Reservoir",
            markerfacecolor="green",
            markersize=10,
        ),
        Line2D(
            [0],
            [0],
            marker="D",
            color="w",
            label="Tank",
            markerfacecolor="orange",
            markersize=10,
        ),
        Line2D([0], [0], color="black", lw=2, label="Pipe"),
        Line2D([0], [0], color="red", lw=2, linestyle="--", label="Pump"),
        Line2D([0], [0], color="purple", lw=2, linestyle=":", label="Valve"),
    ]

    plt.legend(handles=legend_elements, loc='center left', bbox_to_anchor=(1, 0.5))

    plt.title("Water Distribution Network Visualization")
    plt.xlabel("X Coordinate")
    plt.ylabel("Y Coordinate")
    plt.grid(True)
    plt.axis("equal")  # Equal scaling for x and y axes

    # Save the plot to a file
    plt.savefig("network_visualization.png", dpi=300, bbox_inches="tight")


def main():
    inp_file = "dataset/any-town.inp"  # Path to the .inp file
    nodes, links = parse_inp_file(inp_file)
    G = build_graph(nodes, links)
    visualize_network(G)


if __name__ == "__main__":
    main()
