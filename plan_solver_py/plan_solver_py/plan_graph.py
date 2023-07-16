import argparse
import os
import re
import subprocess
import sys
from argparse import Namespace
from pathlib import Path
from typing import Optional

from pydot import Dot, Node, Edge

_CONTENT_PATTERN = r'[^-\s][^-]+[^-\s]'
_PLAN_REGEX = re.compile(
    fr'(?P<coords>{_CONTENT_PATTERN}) --- (?P<name>{_CONTENT_PATTERN}) --- '
    fr'(?:SON: (?P<son>{_CONTENT_PATTERN})|'
    fr'TRUESON: (?P<trueson>{_CONTENT_PATTERN}) --- FALSESON: (?P<falseson>{_CONTENT_PATTERN}))'
)
_COORDS_REGEX = re.compile(
    r'(?P<level>\d+)[|]{2}(?P<index>\d+)'
)


def parse_plan_graph(plan_txt: str, unique_nodes=True, unique_edges=False) -> Dot:
    nodes = [
        Namespace(**match.groupdict()) for match in (
            _PLAN_REGEX.match(line.strip()) for line in plan_txt.splitlines()
        ) if match is not None
    ]

    if unique_nodes:
        def node_name(node) -> str:
            return f"{node.coords} {node.name}"
    else:
        def node_name(node) -> str:
            return node.name

    node_coord_map = {node.coords: node_name(node) for node in nodes}

    if unique_edges:
        edges = set()

        def check_edge(node, coords, label) -> bool:
            edge = (node_name(node), node_coord_map[coords], label)
            if edge not in edges:
                edges.add(edge)
                return True
            return False
    else:
        def check_edge(node, coords, label) -> bool:
            return True

    def try_add_edge(node, coords: str, label: str, **kwargs):
        if coords in node_coord_map and check_edge(node, coords, label):
            graph.add_edge(Edge(src=node_name(node), dst=node_coord_map[coords], label=label, **kwargs))

    graph = Dot("plan_graph", graph_type="digraph")
    for node in nodes:
        graph.add_node(Node(name=node_name(node)))

        if node.son is None:
            try_add_edge(node, node.trueson, label='trueson')
            try_add_edge(node, node.falseson, label='falseson')
        else:
            try_add_edge(node, node.son, label='son')

    return graph


def main():
    with open("/tmp/plan_solver/plan.txt") as f:
        dot = parse_plan_graph(f.read(),
                               unique_nodes=True, unique_edges=False)
        dot.write("/tmp/plan_solver/plan-graph.png", format='png')
        os.system('open /tmp/plan_solver/plan-graph.png')


if __name__ == '__main__':
    main()
