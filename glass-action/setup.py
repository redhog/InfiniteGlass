#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-action',
      version='0.1',
      description='Send window actions from the command line InfiniteGlass',
      long_description='Send window actions from the command line InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "click",
          "pycairo",
          "PyGObject",
      ],
      entry_points={
          'console_scripts': [
              'glass-action = glass_action.main:main',
          ],
      }
  )
