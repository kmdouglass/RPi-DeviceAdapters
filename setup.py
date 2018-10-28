import os
import re
from setuptools import setup, find_packages

# Read the version from the master version file
version_file = os.path.join("tacpho", "adapters", "_version.py")
with open(version_file, "r") as file:
    line = file.readline()
    pattern = re.compile(r"(?:\d+\.){2}\d+")
    version = re.search(pattern, line).group()

config = {
    "version": version,
    "description": "Micro-Manager device adapters for the Raspberry Pi",
    "author": "Kyle M. Douglass",
    "author_email": "kyle.m.douglass@gmail.com",
    "url": "https://github.com/kmdouglass/RPi-DeviceAdapters",
    "download_url": "https://github.com/kmdouglass/RPi-DeviceAdapters",
    "license": "BSD",
    "name": "tacpho.adapters",
    "packages": find_packages(),
    "scripts": ["tacpho/adapters/mm.py"],
    "namespace_packages": ["tacpho"],
}

setup(**config)
