import os
import re
from setuptools import setup

# Read the version from the master version file
version_file = os.path.join("tacpho", "adapters", "_version.py")
with open(version_file, "r") as fh:
    line = fh.readline()
    pattern = re.compile(r"(?:\d+\.){2}\d+(?:-dev)?")
    version = re.search(pattern, line).group()

# Import the README
with open("README.md", "r") as fh:
    long_description = fh.read()

# Parse the requirements file
with open("requirements.txt") as fh:
    requirements = fh.read().splitlines()

config = {
    "version": version,
    "description": "Micro-Manager device adapters for the Raspberry Pi",
    "long_description": long_description,
    "long_description_content_type": "text/markdown",
    "author": "Kyle M. Douglass",
    "author_email": "kyle.m.douglass@gmail.com",
    "url": "https://github.com/kmdouglass/RPi-DeviceAdapters",
    "download_url": "https://github.com/kmdouglass/RPi-DeviceAdapters",
    "license": "BSD",
    "name": "tacpho.adapters",
    "packages": ["tacpho.adapters"],
    "scripts": ["tacpho/adapters/mm.py"],
    "install_requires": requirements,
    "classifiers": [
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: BSD License",
        "Operating System :: POSIX :: Linux",
    ],
}

setup(**config)
