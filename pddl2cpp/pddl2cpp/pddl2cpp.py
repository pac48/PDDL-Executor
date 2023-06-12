import pddl
from pddl import parse_domain, parse_problem
import sys

def main():
    if len(sys.argv) != 2:
        print ('pddl2cpp.py <domain_file>')
        sys.exit(2)
    domain = parse_domain(sys.argv[1])
    pass


if __name__ == '__main__':
    main()
