from setuptools import setup
from setuptools.command.build_py import build_py
from setuptools.command.develop import develop
import pathlib
import subprocess
import shutil
import os
import tempfile

class BuildWithMake(build_py):
    def run(self):
        with tempfile.TemporaryDirectory() as build_temp:
            subprocess.check_call(
                ["make", f"BUILD={build_temp}", f"BINDIR={build_temp}"],
                cwd=pathlib.Path(__file__).parent)

            pkg_src = pathlib.Path(__file__).parent / "glass_annotator"
            (pkg_src / "lib").mkdir(parents=True, exist_ok=True)
            shutil.copy2(pathlib.Path(build_temp) / "glass-annotator", pkg_src / "lib" / "glass-annotator")

        super().run()

class DevelopWithMake(develop):
    def run(self):
        self.run_command("build_py")
        super().run()

setup(
    name="glass-annotator",
    version='0.1',
    description='Application and group ID:s for X application windows',
    long_description='Adds application and group ID:s for X application windows using LD_PRELOAD',
    long_description_content_type="text/markdown",
    author='Egil Moeller',
    author_email='redhog@redhog.org',
    url='https://github.com/redhog/InfiniteGlass',
    packages=["glass_annotator"],
    cmdclass={
        "build_py": BuildWithMake,
        "develop": DevelopWithMake},
    package_data={"glass_annotator": ["lib/*"]},
)
