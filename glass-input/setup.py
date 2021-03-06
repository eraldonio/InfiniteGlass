#!/usr/bin/env python

import setuptools

setuptools.setup(name='glass-input',
      version='0.1',
      description='Input handling (keyboard mapping) for InfiniteGlass',
      long_description='Input handling (keyboard mapping) for InfiniteGlass',
      long_description_content_type="text/markdown",
      author='Egil Moeller',
      author_email='redhog@redhog.org',
      url='https://github.com/redhog/InfiniteGlass',
      packages=setuptools.find_packages(),
      install_requires=[
          "pyyaml",
          "rpdb",
          "numpy",
          "rectangle-packer"
      ],
      entry_points={
          'console_scripts': [
              'glass-input = glass_input:main',
          ],
      },
      package_data={'glass_input': ['*.json']},
      include_package_data=True
  )
