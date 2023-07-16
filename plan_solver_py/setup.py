from setuptools import setup
import glob
import os

package_name = 'plan_solver_py'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
         ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/plan_solver', ['plan_solver/CMakeLists.txt']),
        ('share/' + package_name + '/plan_solver/src', ['plan_solver/src/plan_solver.cpp']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='paul',
    maintainer_email='paulgesel@gmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': ['plan_solver = plan_solver_py.plan_solver:main',
                            'plan_graph = plan_solver_py.plan_graph:main'],
    },
)
