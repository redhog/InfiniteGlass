#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-focus-manager',
      version='0.1',
      description='Focus manager for InfiniteGlass',
      long_description='Focus manager for InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "click"
      ],
      entry_points={
          'console_scripts': [
              'glass-focus-manager = glass_focus_manager.main:main',
          ],
      }
  )
