from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

client = Extension(
    name="pysmlib.client",
    sources=["pysmlib/client.pyx"],
    libraries=["SM", "ICE"]
)
server = Extension(
    name="pysmlib.client",
    sources=["pysmlib/server.pyx"],
    libraries=["SM", "ICE"]
)
helpers = Extension(
    name="pysmlib.helpers",
    sources=["pysmlib/helpers.pyx"],
    libraries=["SM", "ICE"]
)
setup(
    name="pysmlib",
    ext_modules=cythonize([client, server, helpers], language_level=3)
)
