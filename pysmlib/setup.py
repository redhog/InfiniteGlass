from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

smlib_extension = Extension(
    name="pysmlib",
    sources=["pysmlib.pyx"],
    libraries=["SM", "ICE"]
#    library_dirs=["lib"],
#    include_dirs=["lib"]
)
setup(
    name="pysmlib",
    ext_modules=cythonize([smlib_extension], language_level=3)
)
