import argparse
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


def solve_plan_graph(domain_file: str, problem_file: str) -> Optional[str]:
    try:
        result = subprocess.run(
            args=['/home/garrick/.local/bin/Contingent-FF', '-o', Path(domain_file), '-f', Path(problem_file)],
            capture_output=True,
            check=True
        )
    except subprocess.CalledProcessError as e:
        print(f"Error running Contingent-FF subprocess. {e.output}", file=sys.stderr)
        return None

    return result.stdout.decode('utf-8')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', type=Path, default=Path('plan-graph.png'))
    parser.add_argument('--merge-nodes', action='store_true')
    parser.add_argument('--unique-edges', action='store_true')

    subparsers = parser.add_subparsers(dest='command')

    parse_command = subparsers.add_parser('parse')
    parse_command.add_argument('solved_plan', type=argparse.FileType('r'), default='-')

    solve_command = subparsers.add_parser('solve')
    solve_command.add_argument('domain_file', type=Path)
    solve_command.add_argument('problem_file', type=Path)
    solve_command.add_argument('--solver', type=Optional[Path])

    args = parser.parse_args()

    match args.command:
        case 'parse':
            dot = parse_plan_graph(
                args.solved_plan.read(),
                unique_nodes=not args.merge_nodes,
                unique_edges=args.unique_edges
            )
        case 'solve':
            plan_txt = solve_plan_graph(args.domain_file, args.problem_file)
            if plan_txt is None:
                exit(-1)
            dot = parse_plan_graph(
                plan_txt,
                unique_nodes=not args.merge_nodes,
                unique_edges=args.unique_edges
            )
        case _ as cmd:
            print(f"Unknown command '{cmd}'.")
            exit(-1)

    output_format = args.output.suffix[1:]

    if output_format not in {'pdf', 'gif', 'png', 'jpg', 'jpeg', 'svg'}:
        output_format = 'raw'

    dot.write(args.output, format=output_format)


if __name__ == '__main__':
    main()
