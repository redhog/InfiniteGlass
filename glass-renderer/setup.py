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

            pkg_src = pathlib.Path(__file__).parent / "glass_renderer"
            (pkg_src / "bin").mkdir(parents=True, exist_ok=True)
            shutil.copy2(pathlib.Path(build_temp) / "glass-renderer", pkg_src / "bin" / "glass-renderer")

        super().run()

class DevelopWithMake(develop):
    def run(self):
        self.run_command("build_py")
        super().run()

setup(
    name="glass-renderer",
    version='0.1',
    description='The renderer process for InfiniteGlass',
    long_description='A compositor, that is, the opengl rendering pipeline for InfiniteGlass',
    long_description_content_type="text/markdown",
    author='Egil Moeller',
    author_email='redhog@redhog.org',
    url='https://github.com/redhog/InfiniteGlass',
    packages=["glass_renderer"],
    cmdclass={
        "build_py": BuildWithMake,
        "develop": DevelopWithMake},
    package_data={"glass_renderer": ["bin/*"]},
    entry_points={
        "console_scripts": [
            "glass-renderer = glass_renderer:main"
        ]
    },
)
