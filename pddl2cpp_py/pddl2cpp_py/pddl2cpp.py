import argparse
import sys
import os
from jinja2 import Template
import pddl_parser
from ament_index_python.packages import get_package_share_directory


def get_all_templates():
    template_path = os.path.join(get_package_share_directory('pddl2cpp'), 'jinja_templates')
    template_map = {}
    for file_name in [
        f
        for f in os.listdir(template_path)
        if os.path.isfile(os.path.join(template_path, f))
    ]:
        with open(os.path.join(template_path, file_name)) as file:
            template_map[file_name] = file.read()

    return template_map


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("output_file")
    # parser.add_argument("domain_file")
    parser.add_argument('domain_files', nargs='+', help='list of pddl domain files')
    return parser.parse_args()


def main():
    args = parse_args()
    domain_files = args.domain_files
    output_file = args.output_file

    templates = get_all_templates()
    actions_classes = []
    action_names = []
    func_signature_map = dict()
    predicate_map = dict()
    predicates = []
    pred_name_maps = []
    all_types = set()
    for domain_file in domain_files:
        with open(domain_file) as f:
            domain = pddl_parser.parser.parse_domain(f.read())

        all_types = all_types.union(set(domain.types))
        for pred in domain.predicates:
            params = tuple([p.type for p in pred.parameters])
            if params not in func_signature_map:
                func_signature_map[params] = []

            func_signature_map[params].append(pred.name)

        pred_name_maps.append(dict())
        for pred in domain.predicates:
            predicates.append(pred)
            pred_name_maps[-1][pred.name] = pred

        for action in domain.actions:
            j2_template = Template(templates["action.hpp"])
            parameters = [p.name.replace('?', '') for p in action.parameters]

            data = {'domain_name': domain.name, 'class_name': action.name, 'action_str': str(action),
                    'parameters': parameters}
            code = j2_template.render(data, trim_blocks=True)
            actions_classes.append(code)

            class TMP:
                qualified = ""
                underscore = ""

            tmp = TMP()
            tmp.qualified = domain.name + "::" + action.name
            tmp.underscore = domain.name + "_" + action.name
            action_names.append(tmp)

    # predicates = list(predicate_map.values())
    for pred in predicates:
        for param in pred.parameters:
            param.name = param.name.replace('?', '')

    tmp = {f"{p.name}:::{str([param.type for param in p.parameters])}": p for p in predicates}
    predicates = list(tmp.values())

    func_signatures = [list(p) for p in list(func_signature_map.keys())]

    j2_template = Template(templates["bt_actions.hpp"])
    data = {'action_classes': "\n\n".join(actions_classes), 'action_names': action_names,
            'predicates': predicates, 'types': list(all_types), 'func_signatures': func_signatures}
    code = j2_template.render(data, trim_blocks=True)

    with open(output_file, 'w') as f:
        f.write(code)


if __name__ == '__main__':
    main()
