#!/usr/bin/env python

import setuptools
from setuptools.command.install import install
from setuptools.command.develop import develop
import os.path

setuptools.setup(
    name='glass-theme',
    version='0.1',
    description='theme for the overlay layer for InfiniteGlass',
    long_description='theme for the overlay layer for InfiniteGlass',
    long_description_content_type="text/markdown",
    author='Egil Moeller',
    author_email='redhog@redhog.org',
    url='https://github.com/redhog/InfiniteGlass',
    packages=setuptools.find_packages(),
    install_requires=[
    ],
    entry_points={
        'console_scripts': [
            'glass-theme = glass_theme:main',
        ],
    },
    package_data={'glass_theme': ['*.glsl']},
    include_package_data=True
)
