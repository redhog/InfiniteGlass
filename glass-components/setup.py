#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-components',
      version='0.1',
      description='Components manager for InfiniteGlass',
      long_description='Components manager for InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "click",
          "pyyaml",
          "python-slugify",
          "rpdb"
      ],
      entry_points={
          'console_scripts': [
              'glass-components = glass_components.main:main',
          ],
      }
  )
