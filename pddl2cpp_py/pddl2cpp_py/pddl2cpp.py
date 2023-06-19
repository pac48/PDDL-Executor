import argparse
import sys
import os
from jinja2 import Template
import pddl_parser


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
    actions_names = []
    predicates = []
    predicates_map = dict()

    for domain_file in domain_files:
        with open(domain_file) as f:
            domain = pddl_parser.parser.parse_domain(f.read())

        for pred in domain.predicates:
            params = tuple([p.type for p in pred.parameters])
            if params not in predicates_map:
                predicates_map[params] = []

            predicates_map[params].append(pred.name)

        predicates.extend([v for v in domain.predicates])
        for action in domain.actions:
            j2_template = Template(templates["action.hpp"])
            parameters = [p.name.replace('?', '') for p in action.parameters]

            data = {'class_name': action.name, 'action_str': str(action), 'parameters': parameters}
            code = j2_template.render(data, trim_blocks=True)
            actions_classes.append(code)
            actions_names.append(action.name)

    for pred in predicates:
        for param in pred.parameters:
            param.name = param.name.replace('?', '')

    func_signatures = [list(p) for p in list(predicates_map.keys())]

    j2_template = Template(templates["bt_actions.hpp"])
    data = {'action_classes': "\n\n".join(actions_classes), 'action_names': actions_names,
            'predicates': list(set(predicates)), 'types': list(domain.types), 'func_signatures': func_signatures}
    code = j2_template.render(data, trim_blocks=True)

    with open(output_file, 'w') as f:
        f.write(code)


if __name__ == '__main__':
    main()
