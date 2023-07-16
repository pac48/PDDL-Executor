import os
from ament_index_python.packages import get_package_share_directory
import argparse
import shutil

import hashlib


def hash_file(domain_file, problem_file):
    sha256_hash = hashlib.sha256()
    with open(domain_file, 'rb') as file:
        # Read the file in chunks to handle large files efficiently
        for chunk in iter(lambda: file.read(4096), b''):
            sha256_hash.update(chunk)
    with open(problem_file, 'rb') as file:
        # Read the file in chunks to handle large files efficiently
        for chunk in iter(lambda: file.read(4096), b''):
            sha256_hash.update(chunk)

    return sha256_hash.hexdigest()


def main():
    parser = argparse.ArgumentParser(description='Argument parser for domain and problem PDDL files.')
    parser.add_argument('-o', '--domain', help='Path to domain PDDL file', required=True)
    parser.add_argument('-f', '--problem', help='Path to problem PDDL file', required=True)
    args = parser.parse_args()

    domain_file = args.domain
    problem_file = args.problem

    h_val = hash_file(domain_file, problem_file)
    planner_path = f'/tmp/plan_solver/plan_solver_{h_val}'
    if not os.path.exists(planner_path):
        build_dir = "/tmp/plan_solver/build"
        file_path = os.path.join(get_package_share_directory('plan_solver_py'), 'plan_solver')

        if not os.path.exists(build_dir):
            os.makedirs(build_dir, exist_ok=True)

        cmd = f"cd {build_dir} && cmake {file_path} -DPDDL_PROBLEM={problem_file} -DPDDL_DOMAIN={domain_file} && make"
        os.system(cmd)

        shutil.move(os.path.join(build_dir, 'plan_solver'), planner_path)
        shutil.rmtree(build_dir)

    cmd = f"{planner_path} {problem_file}"
    os.system(cmd)


if __name__ == '__main__':
    main()
