#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-lock',
      version='0.1',
      description='Screen locker controller for InfiniteGlass',
      long_description='Wraps various types of screen lockers in a standard protocol for lock/unlock.',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "click",
      ],
      entry_points={
          'console_scripts': [
              'glass-lock = glass_lock.main:main',
          ],
          'InfiniteGlass.actions': [
              "screen_lock = glass_lock.main:screen_lock",
          ],
          'InfiniteGlass.lockers': [
              "i3 = glass_lock.cmdlockers:LockerI3",
              "xscreensaver = glass_lock.cmdlockers:LockerXScreensaver",
              "gnome = glass_lock.dbus_lockers:LockerGnome",
              "mate = glass_lock.dbus_lockers:LockerMate",
              "xfce = glass_lock.dbus_lockers:LockerXfce4",
              "light = glass_lock.dbus_lockers:LockerLight",
          ]
      }
  )
