#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-session-wrapper',
      version='0.1',
      description='Wraps applications that do not support the session manager protocol',
      long_description='Wraps applications that do not support the session manager protocol but that support saving their state on exit to a session directory, e.g. like "chromium-browser --user-data-dir=...". The application needs to set WM_CLIENT_MACHINE and _NET_WM_PID.',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
      ],
      entry_points={
          'console_scripts': [
              'glass-session-wrapper = glass_session_wrapper:main',
          ],
      }
  )
