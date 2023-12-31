from setuptools import find_packages
from setuptools import setup

package_name = 'pddl2cpp_py'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(),
    data_files=[
        ('share/ament_index/resource_index/packages',
         ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools', 'jinja2'],
    zip_safe=True,
    maintainer='paul',
    maintainer_email='paulgesel@gmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': ['pddl2cpp = pddl2cpp_py.pddl2cpp:main',
                            'pddl2search = pddl2cpp_py.pddl2search:main'
                            ],
    },
)
