#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-islands',
      version='0.1',
      description='Desktop islands for InfiniteGlass',
      long_description='Desktop islands for InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "pyyaml",
          "python-slugify"
      ],
      entry_points={
          'console_scripts': [
              'glass-islands = glass_islands.main:main',
          ],
      },
      package_data={'glass_islands': ['*.svg', '*.json']},
      include_package_data=True
  )
