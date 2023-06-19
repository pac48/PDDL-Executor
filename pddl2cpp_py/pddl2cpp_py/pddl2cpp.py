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

    for domain_file in domain_files:
        with open(domain_file) as f:
            domain = pddl_parser.parser.parse_domain(f.read())
        predicates.extend([v.name for v in domain.predicates])
        for action in domain.actions:
            j2_template = Template(templates["action.hpp"])
            parameters = [p.name.replace('?', '') for p in action.parameters]

            data = {'class_name': action.name, 'action_str': str(action), 'parameters': parameters}
            code = j2_template.render(data, trim_blocks=True)
            actions_classes.append(code)
            actions_names.append(action.name)

    j2_template = Template(templates["bt_actions.hpp"])
    data = {'action_classes': "\n\n".join(actions_classes), 'action_names': actions_names,
            'predicates': list(set(predicates))}
    code = j2_template.render(data, trim_blocks=True)

    with open(output_file, 'w') as f:
        f.write(code)


if __name__ == '__main__':
    main()
