#!/usr/bin/env python

import setuptools
from setuptools.command.install import install
from setuptools.command.develop import develop
import os.path

setuptools.setup(
    name='glass-config-init',
    version='0.1',
    description='Initializes ~/.config/glass',
    long_description='Initializes ~/.config/glass with a default set of configuration files',
    long_description_content_type="text/markdown",
    author='Egil Moeller',
    author_email='redhog@redhog.org',
    url='https://github.com/redhog/InfiniteGlass',
    packages=setuptools.find_packages(),
    install_requires=[
    ],
    entry_points={
        'console_scripts': [
            'glass-config-init = glass_config_init:main',
        ],
    },
    package_data={'glass_config_init': ['*']},
    include_package_data=True
)
