#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-widgets',
      version='0.1',
      description='Widgets for the overlay layer for InfiniteGlass',
      long_description='Widgets for the overlay layer for InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
      ],
      entry_points={
          'console_scripts': [
              'glass-widgets = glass_widgets:main',
          ],
      },
      include_package_data=True
  )
