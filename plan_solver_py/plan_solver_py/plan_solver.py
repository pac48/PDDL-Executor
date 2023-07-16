import os
from ament_index_python.packages import get_package_share_directory

def main():
    file_path = os.path.join(get_package_share_directory('plan_solver_py'), 'plan_solver')
    print(file_path)


if __name__ == '__main__':
    main()
