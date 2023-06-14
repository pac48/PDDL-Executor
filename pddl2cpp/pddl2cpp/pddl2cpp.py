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


def main():
    if len(sys.argv) != 2:
        print('pddl2cpp.py <domain_file>')
        sys.exit(2)
    with open(sys.argv[1]) as f:
        domain = pddl_parser.parser.parse_domain(f.read())

    templates = get_all_templates()

    actions_content = []
    for action in domain.actions:
        j2_template = Template(templates["action.hpp"])
        parameters = [p.name.replace('?', '') + '_' + p.type for p in action.parameters]

        data = {'class_name': action.name, 'parameters': parameters}
        code = j2_template.render(data, trim_blocks=True)
        actions_content.append(code)

    j2_template = Template(templates["bt_actions.hpp"])
    data = {'actions': "\n\n".join(actions_content)}
    code = j2_template.render(data, trim_blocks=True)

    with open('tmp.hpp', 'w') as f:
        f.write(code)
    # print(code)


if __name__ == '__main__':
    main()
