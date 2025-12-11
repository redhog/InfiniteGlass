#!/usr/bin/env python

import setuptools
import os.path

setuptools.setup(
    name='glass-world-splash',
    version='0.1',
    description='Splash screen lock/unlock animation for InfiniteGlass',
    long_description='Splash screen lock/unlock animation for InfiniteGlass',
    long_description_content_type="text/markdown",
    author='Egil Moeller',
    author_email='redhog@redhog.org',
    url='https://github.com/redhog/InfiniteGlass',
    packages=setuptools.find_packages(),
    install_requires=[
    ],
    entry_points={
        'console_scripts': [
            'glass-world-splash = glass_world_splash.main:main',
        ],
        'InfiniteGlass.actions': [
            "splash_unlock = glass_world_splash.main:splash_unlock",
            "splash_lock = glass_world_splash.main:splash_lock",
        ]
    },
    package_data={'glass_input': ['*.json']},
    include_package_data=True,
)
