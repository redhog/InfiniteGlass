#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-ghosts',
      version='0.1',
      description='Window ghosts / shadows for InfiniteGlass',
      long_description='Window ghosts / shadows for InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "pyyaml"
      ],
      entry_points={
          'console_scripts': [
              'glass-ghosts = glass_ghosts.main:main',
          ],
      },
      package_data={'glass_ghosts': ['*.svg', '*.json']},
      include_package_data=True
  )
