import setuptools
import setuptools.dist

#setuptools.dist.Distribution().fetch_build_eggs(['Cython>=0.15.1'])

from Cython.Build import cythonize

client = setuptools.Extension(
    name="pysmlib.client",
    sources=["pysmlib/client.pyx"],
    libraries=["SM", "ICE"]
)
server = setuptools.Extension(
    name="pysmlib.server",
    sources=["pysmlib/server.pyx"],
    libraries=["SM", "ICE"]
)
ice = setuptools.Extension(
    name="pysmlib.ice",
    sources=["pysmlib/ice.pyx"],
    libraries=["SM", "ICE"]
)
helpers = setuptools.Extension(
    name="pysmlib.helpers",
    sources=["pysmlib/helpers.pyx"],
    libraries=["SM", "ICE"]
)
setuptools.setup(
    name="pysmlib",

    version='0.1',
    description='A python binding for X11 SMlib and ICElib',
    long_description="""A python binding for the session management protocol for X11 as well as some of the Inter Client Exchange protocol.""",
    long_description_content_type="text/markdown",
    author='Egil Moeller',
    author_email='redhog@redhog.org',
    url='https://github.com/redhog/InfiniteGlass',
    packages=setuptools.find_packages(),
    install_requires=[],
    ext_modules=cythonize([client, server, ice, helpers], language_level=3),
    include_package_data=True
)
