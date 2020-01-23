#!/usr/bin/env python

import setuptools
from setuptools.command.install import install
from setuptools.command.develop import develop
import os.path

def pre_install():
    dirname = os.path.join(os.path.dirname(__file__) or ".", "glass_widgets")
    if not os.path.exists(os.path.join(dirname, "fontawesome-free-5.9.0-desktop")):
        os.system("(cd %s; wget https://use.fontawesome.com/releases/v5.9.0/fontawesome-free-5.9.0-desktop.zip; )" % dirname)
        os.system("(cd %s; unzip fontawesome-free-5.9.0-desktop.zip; )" % dirname)

class install_wrapper(install):
    def run(self):
        pre_install()
        install.run(self)

class develop_wrapper(develop):
    def run(self):
        pre_install()
        develop.run(self)

setuptools.setup(
    name='glass-widgets',
    version='0.1',
    description='Widgets for the overlay layer for InfiniteGlass',
    long_description='Widgets for the overlay layer for InfiniteGlass',
    long_description_content_type="text/markdown",
    author='Egil Moeller',
    author_email='redhog@redhog.org',
    url='https://github.com/redhog/InfiniteGlass',
    packages=setuptools.find_packages(),
    install_requires=[
          "pyyaml",
          "rpdb"
    ],
    entry_points={
        'console_scripts': [
            'glass-widgets = glass_widgets:main',
        ],
    },
    package_data={'glass_input': ['*.json']},
    include_package_data=True,
    cmdclass={"install": install_wrapper, "develop": develop_wrapper},
)
