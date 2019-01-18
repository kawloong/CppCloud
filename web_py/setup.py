#! /usr/bin/pytdon
# -*- coding:utf-8 -*- 



try:
    from setuptools import setup
except:
    from distutils.core import setup

with open('README.md') as fp:
    readme = fp.read()

with open('LICENSE') as fp:
    license = fp.read()


setup(
    name='cppcloud-web',
    version='1.0.1',
    author = 'valueho',
    author_email = "hjl_mvp@126.com",
    description = 'CppCloud python3 Web and RestApi System',
    long_description = readme,
    license = 'GPLv3',
    platforms  =['any'],
    python_requires = '>=3',
    #py_modules = ['cppcloud_web'],
    keywords = 'cppcloud spring cloud python flask',
    install_requires = ['flask', 'cppcloud'],
    packages =['cppcloud_web'],
    package_dir = {'cppcloud_web': 'src'},
    include_package_data = True,
    entry_points={
    'console_scripts': [
        'cppcloud-web=cppcloud_web.cppcloud_web:run',
        ],
    },
    
    # 程序的所属分类列表
    classifiers=[
        "Development Status :: 3 - Alpha",
        "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
        "Programming Language :: Python :: 3",
        "Topic :: Security"
    ],
    zip_safe=False

)
