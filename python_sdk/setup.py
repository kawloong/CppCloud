from setuptools import setup, find_packages

with open('README.md') as fp:
    readme = fp.read()

with open('LICENSE') as fp:
    license = fp.read()


setup(
    name='cppcloud',
    version='1.0',
    author = 'valueho',
    author_email = "valueho@gmail.com",
    description = 'CppCloud python3 客户端sdk',
    long_description=readme,
    license=license,
    platforms=['any'],

    packages = ['cppcloud'],
    #package_dir = {'xcppcloud': 'cppcloud'},
    install_requires = ['requests']

)