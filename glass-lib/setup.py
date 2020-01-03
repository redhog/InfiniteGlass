#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-lib',
      version='0.1',
      description='A simplification layer on top of Xlib',
      long_description="""A simplification layer on top of Xlib mainly intended for writing InfiniteGlass clients/modules.""",
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "numpy",
          "python-xlib>=0.25",
          "sakstig",
      ],
      include_package_data=True
  )
