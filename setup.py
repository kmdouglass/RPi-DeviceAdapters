import os
import re
from setuptools import setup

# Read the version from the master version file
version_file = os.path.join("tacpho", "adapters", "_version.py")
with open(version_file, "r") as file:
    line = file.readline()
    pattern = re.compile(r"(?:\d+\.){2}\d+(?:-dev)?")
    version = re.search(pattern, line).group()

# Parse the requirements file
with open("requirements.txt") as file:
    requirements = file.read().splitlines()

config = {
    "version": version,
    "description": "Micro-Manager device adapters for the Raspberry Pi",
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
