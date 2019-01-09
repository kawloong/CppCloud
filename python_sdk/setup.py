#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 

from setuptools import setup, find_packages

import sys
if sys.version_info < (3, 2):
    sys.exit('Python 3.2 or greater is required.')
 
try:
    from setuptools import setup
except:
    from distutils.core import setup

with open('README.md') as fp:
    readme = fp.read()

with open('LICENSE') as fp:
    license = fp.read()


setup(
    name='cppcloud',
    version='1.1.0',
    author = 'valueho',
    author_email = "valueho@gmail.com",
    description = 'CppCloud python3 客户端sdk',
    long_description=readme,
    license="GPLv3",
    platforms=['any'],

    packages = ['cppcloud'],
    #package_data = {'': ['.md', ]},
    data_files = ['LICENSE', 'README.md'],
    #package_dir = {'xcppcloud': 'cppcloud'},
    install_requires = ['requests'],
  
    # 程序的所属分类列表
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Topic :: Utilities",
        "License :: OSI Approved :: GNU General Public License (GPL)"
    ],
    zip_safe=False
)