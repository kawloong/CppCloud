#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 


import sys


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
    version='1.0.3',
    author = 'valueho',
    author_email = "hjl_mvp@126.com",
    description = 'CppCloud python3 客户端sdk',
    long_description=readme,
    license='GPLv3',
    platforms=['any'],
    python_requires = '>=3',

    packages = ['cppcloud'],
    #package_dir = {'xcppcloud': 'cppcloud'},

    include_package_data=True,
    install_requires = ['requests'],
    
    # 程序的所属分类列表
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Topic :: Utilities",
        "License :: OSI Approved :: GNU General Public License (GPL)"
    ],
    zip_safe=False

)
