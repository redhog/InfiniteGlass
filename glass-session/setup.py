#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-session',
      version='0.1',
      description='Session root process for InfiniteGlass',
      long_description='Session root process for InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "pyyaml",
          "python-slugify",
          "rpdb"
      ],
      entry_points={
          'console_scripts': [
              'glass-session = glass_session.main:main',
          ],
      },
  )
