import os

# import pddl_parser.parser as p
import pddl_parser
from pddl_parser import *
from ament_index_python.packages import get_package_share_directory

param1 = pddl_parser.parser.Parameter("?r", "robot")
param2 = pddl_parser.parser.Parameter("?loc", "landmark")
pred = pddl_parser.parser.Predicate("robot_at", [param1, param2])
tmp = pred.parameters
pred.parameters = [param1]

print(pred)

pred = pddl_parser.parser.parse_predicate("(person_at ?p - person ?lm - landmark)")
print(pred)

domain_dir = os.path.join(get_package_share_directory("pddl_parser_py"), "tests", "pddl", "medicine_domain.pddl")
with open(domain_dir) as f:
    content = f.read()
    domain = pddl_parser.parser.parse_domain(content)
print(domain)

action = pddl_parser.parser.parse_action("""
(:action detectPerson
    :parameters (?r - robot ?p - person ?loc - landmark)
    :precondition (and
    			(robot_at ?r ?loc)
			(not (init_move_to_landmark))
			(not (init_guide_person_to_landmark_attempt))
   		 )
    :observe (person_at ?p ?loc)
)""")
print(action)