import argparse
import sys
import os
from jinja2 import Template
import pddl_parser
from ament_index_python.packages import get_package_share_directory
from collections import defaultdict
import itertools


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
    parser.add_argument('domain_file', help='domain file')
    parser.add_argument('problem_file', help='problem file')
    return parser.parse_args()


class ActionInstance:
    def __init__(self):
        self.name = ""
        self.base_name = ""
        self.pre = ""
        self.effect = ""
        self.observe = ""
        self.params = ""


def main():
    args = parse_args()
    domain_file = args.domain_file
    problem_file = args.problem_file
    output_file = args.output_file

    templates = get_all_templates()
    with open(domain_file) as f:
        domain = pddl_parser.parser.parse_domain(f.read())
    with open(problem_file) as f:
        problem = pddl_parser.parser.parse_problem(f.read())

    objs = domain.constants
    for obj in problem.objects:
        objs.append(obj)
    problem.objects = list(set(objs))
    type_counter = defaultdict(int)
    for obj in problem.objects:
        type_counter[obj.type] = type_counter[obj.type]+1
        obj.name = obj.type + str(type_counter[obj.type])

    domain_types = set(domain.types)
    domain_types.add('')  # add empty type
    problem_objects_map = defaultdict(list)
    for obj in problem.objects:
        problem_objects_map[obj.type].append(obj.name)

    problem_types = set(problem_objects_map.keys())
    assert (problem_types.issubset(domain_types))
    # get all possible action instances
    kb_template = []
    kb_template_map = dict()
    counter = 0
    for pred in domain.predicates:
        params = pred.parameters
        objects = []
        for param in params:
            objects.append(problem_objects_map[param.type])
        parameter_product = list(itertools.product(*objects))
        for val in parameter_product:
            param_subs = {pair[0].name.strip('?'): pair[1] for pair in zip(pred.parameters, val)}
            pred_inst = pddl_parser.parser.instantiate_predicate(pred, param_subs)

            kb_template.append(pred_inst)
            kb_template_map[str(pred_inst)] = counter
            counter += 1

    # get all possible action instances
    all_actions = []
    all_observe_actions = []
    for action in domain.actions:
        params = action.parameters
        objects = []
        for param in params:
            # tmp = [(param.type, v) for v in problem_objects_map[param.type]]
            tmp = [v for v in problem_objects_map[param.type]]
            objects.append(tmp)
        parameter_product = list(itertools.product(*objects))
        for param in parameter_product:
            param_subs = {pair[0].name.strip('?'): pair[1] for pair in zip(action.parameters, param)}
            action_inst = pddl_parser.parser.instantiate_action(action, param_subs, set(problem.objects))

            param_str = "_".join(p for p in param)
            param_str = "_" + param_str

            act = ActionInstance()
            act.name = action.name + param_str
            act.name = act.name.replace('-', '_')
            act.base_name = action.name
            act.params = param

            if action.name == "MessageGivenSuccess":
                pass
            act.pre = parse_preconditions(action_inst.precondtion, False,
                                          kb_template_map) + parse_observe_preconditions(
                action_inst.observe, kb_template_map) + ';'  # + " && " + tmp + ';'
            act.effect = parse_effect(action_inst.effect, kb_template_map)
            if len(action_inst.observe.conditions) > 0:
                act.observe = parse_observe(action_inst.observe, kb_template_map)
                all_observe_actions.append(act)
            else:
                all_actions.append(act)

    j2_template = Template(templates["pddl_data.hpp"])

    if (len(kb_template) % 8) == 0:
        size_kb_data = len(kb_template)
    else:
        size_kb_data = 8 + 8 * (len(kb_template) // 8)

    indexers = []
    for val in kb_template:
        val = str(val)
        val = val.replace(',', '_')
        val = val.replace('-', '_')
        val = val.replace(' ', '')
        val = val.replace('(', '')
        val = val.replace(')', '')
        val = val.replace("'", '')
        indexers.append(val)

    problem_initialization = "\n".join("" + f'func_map["{pred}"] = {index};' for pred, index in kb_template_map.items())

    data = {'size_kb_data': size_kb_data, 'actions': all_actions, 'observe_actions': all_observe_actions,
            'indexers': indexers, 'problem_initialization': problem_initialization}
    code = j2_template.render(data, trim_blocks=True)

    with open(output_file, 'w') as f:
        f.write(code)


def parse_effect(cond, kb_template_map, truth_val=True):
    if cond.op == pddl_parser.parser.NOT:
        assert (len(cond.conditions) == 1)
        assert (type(cond.conditions[0]) is pddl_parser.parser.InstantiatedPredicate)
        index = kb_template_map[str(cond.conditions[0])]
        out = f"state.data[{index}] = 0;\n"
    elif cond.op == pddl_parser.parser.AND:
        out = ''
        for (ind, sub_cond) in enumerate(cond.conditions):
            if type(sub_cond) is pddl_parser.parser.InstantiatedPredicate:
                index = kb_template_map[str(sub_cond)]
                out += f"state.data[{index}] = 1;\n"
            else:
                out += f"{parse_effect(sub_cond, kb_template_map)}"
    elif cond.op == pddl_parser.parser.WHEN:
        assert (len(cond.conditions) == 2)
        if type(cond.conditions[0]) is pddl_parser.parser.InstantiatedPredicate:
            index1 = kb_template_map[str(cond.conditions[0])]
            cause = f"state.data[{index1}]==1"
        else:
            cause = parse_preconditions(cond.conditions[0], False, kb_template_map)
        if type(cond.conditions[1]) is pddl_parser.parser.InstantiatedPredicate:
            index2 = kb_template_map[str(cond.conditions[1])]
            effect = f" state.data[{index2}]=1; "
        else:
            effect = parse_effect(cond.conditions[1], kb_template_map)
        out = f"if (" + cause + ")" + "{" + effect + "}"
    else:
        raise Exception("wrong condition for condition")

    return out


def parse_observe(cond, kb_template_map):
    out = ""
    assert (cond.op == pddl_parser.parser.AND)
    assert (len(cond.conditions) == 1)
    pred = cond.conditions[0]
    assert (type(cond.conditions[0]) is pddl_parser.parser.InstantiatedPredicate)
    index = kb_template_map[str(pred)]
    out += f"state1.data[{index}] = 1;\n"
    out += f"state2.data[{index}] = 0;\n"
    # if len(cond.conditions) == 1:
    #     cond = cond.conditions[0]
    #
    # if len(cond.conditions) > 0:
    #     raise Exception(f"observe only support predicates")

    return out


def parse_observe_preconditions(cond, kb_template_map):
    if len(cond.conditions) == 0:
        return ""
    assert (len(cond.conditions) == 1)
    assert (cond.op == pddl_parser.parser.AND)
    pred = cond.conditions[0]
    index = kb_template_map[str(pred)]
    return f" && (state.data[{index}]==2)"


# def parse_preconditions_unknowns(cond, kb_template_map):
#     out = ""
#     for (ind, pred) in enumerate(cond.predicates):
#         index = kb_template_map[str(pred)]
#         if ind == 0:
#             delim = ""
#         else:
#             delim = " && "
#         out += f"{delim}state.data[{index}]!=2"
#
#     if len(cond.conditions) > 0 and len(cond.predicates) > 0:
#         out += " && "
#     for (ind, sub_cond) in enumerate(cond.conditions):
#         if ind == 0:
#             delim = ""
#         else:
#             delim = " && "
#         out += f"{delim}{parse_preconditions_unknowns(sub_cond, kb_template_map)}"
#
#     return out


def parse_preconditions(cond, negate, kb_template_map):
    if cond.op == pddl_parser.parser.NOT:
        assert (len(cond.conditions) == 1)
        if type(cond.conditions[0]) is pddl_parser.parser.InstantiatedCondition:
            out = parse_preconditions(cond.conditions[0], not negate, kb_template_map)
        else:
            out = "("
            index = kb_template_map[str(cond.conditions[0])]
            if not negate:
                out += f"state.data[{index}]==0"
            else:
                out += f"state.data[{index}]==1"
            out += ')'
    elif cond.op == pddl_parser.parser.AND:
        out = ""
        for (ind, sub_cond) in enumerate(cond.conditions):
            if ind == 0:
                delim = ""
            else:
                if not negate:
                    delim = " && "
                else:
                    delim = " || "
            if type(sub_cond) is pddl_parser.parser.InstantiatedPredicate:
                index = kb_template_map[str(sub_cond)]
                if not negate:
                    out += f"{delim}state.data[{index}]==1"
                else:
                    out += f"{delim}state.data[{index}]==0"
            else:
                out += f"{delim}({parse_preconditions(sub_cond, negate, kb_template_map)})"
        # if len(cond.conditions) > 0 and len(cond.predicates) > 0:
        #     if not negate:
        #         out += " && "
        #     else:
        #         out += " || "
        # for (ind, sub_cond) in enumerate(cond.conditions):
        #     if ind == 0:
        #         delim = ""
        #     else:
        #         if not negate:
        #             delim = " && "
        #         else:
        #             delim = " || "
        if len(out) == 0:
            return "true"

        out = '(' + out + ')'

        if len(out) > 0:
            out = '(' + out + ')'
        else:
            assert (0)
    else:
        raise Exception("wrong condition type")

    return out


if __name__ == '__main__':
    main()
