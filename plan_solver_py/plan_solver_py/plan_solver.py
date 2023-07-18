import os
import sys
import argparse
import shutil
import hashlib
import time

from jinja2 import Template

from ament_index_python.packages import get_package_share_directory
import pddl_parser


def hash_file(domain_file, problem_file):
    sha256_hash = hashlib.sha256()
    with open(domain_file, 'rb') as file:
        # Read the file in chunks to handle large files efficiently
        for chunk in iter(lambda: file.read(4096), b''):
            sha256_hash.update(chunk)

    with open(problem_file) as f:
        problem = pddl_parser.parser.parse_problem(f.read())

    objs = ":".join(str(obj) for obj in problem.objects)
    sha256_hash.update(objs.encode())

    return sha256_hash.hexdigest()


class Node:
    def __init__(self, id, action, true_son=None, false_son=None, son=None):
        self.id = id
        self.action = action
        self.true_son = true_son
        self.false_son = false_son
        self.son = son

    def __repr__(self):
        return " ".join(str(v) for v in [self.id, self.action, self.true_son, self.false_son, self.son])


class Graph:
    def __init__(self):
        self.nodes = {}

    def add_node(self, id, action, true_son=None, false_son=None, son=None):
        self.nodes[id] = Node(id, action, true_son, false_son, son)


def parse_graph(input_lines):
    graph = Graph()
    input_lines = input_lines.split("\n")
    for line in input_lines:
        if line == '-------------------------------------------------' or len(line) == 0:
            continue
        parts = line.split(' --- ')
        id = parts[0]
        action = parts[1]

        if "TRUESON" in parts[2]:
            true_son = parts[2].split(": ")[1]
            false_son = parts[3].split(": ")[1]
            graph.add_node(id, action, true_son=true_son, false_son=false_son)
        else:
            son = parts[2].split(": ")[1]
            graph.add_node(id, action, son=son)

    return graph


def get_all_templates():
    template_path = os.path.join(
        os.path.dirname(__file__), "jinja_templates"
    )
    template_map = {}
    for file_name in [
        f
        for f in os.listdir(template_path)
        if os.path.isfile(os.path.join(template_path, f))
    ]:
        with open(os.path.join(template_path, file_name)) as file:
            template_map[file_name] = file.read()

    return template_map


def add_action_sequence(action, action_map, templates):
    j2_template = Template(templates["action.xml"])

    ports = ""
    parts = action.split(" ")
    action_inst = action_map[parts[0]]
    action_name = action_inst.name
    if len(parts) > 0:
        params = parts[1:]
        ports = ""
        sep = ""
        for i in range(len(params)):
            param = params[i]
            inst_name = action_inst.parameters[i].name
            inst_name = inst_name.replace('?', '')
            ports += sep + inst_name + "=\"" + param + "\""
            sep = " "

    action_name_underscore = action_name.replace("::", '_')
    data = {"action_id": action_name, "action_name": action_name,
            'action_name_underscore': action_name_underscore, "ports": ports}
    return j2_template.render(data, trim_blocks=True) + "\n"


def add_observe_action_sequence(observe_result, action, action_map, templates):
    if not observe_result:
        return ""

    j2_template = Template(templates["action.xml"])

    ports = ""
    parts = action.split(" ")
    action_inst = action_map[parts[0]]
    action_name = action_inst.name
    if len(parts) > 0:
        params = parts[1:]
        ports = ""
        sep = ""
        for i in range(len(params)):
            param = params[i]
            inst_name = action_inst.parameters[i].name
            inst_name = inst_name.replace('?', '')
            ports += sep + inst_name + "=\"" + param + "\""
            sep = " "

    action_name_underscore = action_name.replace("::", '_')
    data = {"action_id": action_name, "action_name": action_name,
            'action_name_underscore': action_name_underscore, "ports": ports}
    return j2_template.render(data, trim_blocks=True) + "\n"


def get_sub_tree(node, graph, action_map, templates):
    if node.true_son and node.false_son:
        assert (node.false_son in graph.nodes and node.false_son in graph.nodes)
        action_sequence_1 = add_observe_action_sequence(True, node.action, action_map, templates) + get_sub_tree(
            graph.nodes[node.true_son],
            graph, action_map, templates)

        action_sequence_2 = add_observe_action_sequence(False, node.action, action_map, templates) + \
                            get_sub_tree(graph.nodes[node.false_son], graph, action_map, templates)

        data = {'action_sequence_1': action_sequence_1, 'action_sequence_2': action_sequence_2}
        j2_template = Template(templates["observe_sequence.xml"])
        return j2_template.render(data, trim_blocks=True)

    else:
        out = add_action_sequence(node.action, action_map, templates)
        if node.son in graph.nodes:
            out += get_sub_tree(graph.nodes[node.son], graph, action_map, templates)
        return out


def generate_bt(plan_file, bt_file, domain):
    if os.path.exists(plan_file):
        action_map = {}
        for action in domain.actions:
            action_map[action.name] = action
            action_map[action.name].name = domain.name + "::" + action.name

        with open(plan_file) as f:
            graph = parse_graph(f.read())

        templates = get_all_templates()

        tree_str = get_sub_tree(graph.nodes['0||0'], graph, action_map, templates)

        data = {'tree': tree_str}
        j2_template = Template(templates["bt.xml"])
        tree = j2_template.render(data, trim_blocks=True)
        with open(bt_file, 'w') as f:
            f.write(tree)


def main():
    parser = argparse.ArgumentParser(description='Argument parser for domain and problem PDDL files.')
    parser.add_argument('-o', '--domain', help='Path to domain PDDL file', required=True)
    parser.add_argument('-f', '--problem', help='Path to problem PDDL file', required=True)
    args = parser.parse_args()

    domain_file = args.domain
    if not os.path.isabs(domain_file):
        raise AssertionError("--domain must be an absolute path")
    problem_file = args.problem
    if not os.path.isabs(problem_file):
        raise AssertionError("--problem must be an absolute path")

    plan_file = "/tmp/plan_solver/plan.txt"
    bt_file = "/tmp/plan_solver/bt.xml"
    while os.path.exists(plan_file):
        os.remove(plan_file)
    while os.path.exists(bt_file):
        os.remove(bt_file)

    h_val = hash_file(domain_file, problem_file)
    planner_path = f'/tmp/plan_solver/plan_solver_{h_val}'
    if not os.path.exists(planner_path):
        build_dir = "/tmp/plan_solver/build"
        file_path = os.path.join(get_package_share_directory('plan_solver_py'), 'plan_solver')

        if not os.path.exists(build_dir):
            os.makedirs(build_dir, exist_ok=True)

        cmd = f"cd {build_dir} && cmake {file_path} -DCMAKE_BUILD_TYPE=Release -DPDDL_PROBLEM={problem_file} -DPDDL_DOMAIN={domain_file} && make"
        os.system(cmd)

        if os.path.exists('/tmp/plan_solver/include'):
            shutil.rmtree('/tmp/plan_solver/include')
        shutil.move(os.path.join(build_dir, 'plan_solver'), planner_path)
        shutil.move(os.path.join(build_dir, 'pddl_problem', 'include'), '/tmp/plan_solver')
        shutil.rmtree(build_dir)


    cmd = f"{planner_path} {problem_file}"
    os.system(cmd)

    with open(domain_file) as f:
        domain = pddl_parser.parser.parse_domain(f.read())
    generate_bt(plan_file, bt_file, domain)


if __name__ == '__main__':
    main()
