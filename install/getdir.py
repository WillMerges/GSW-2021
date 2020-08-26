#!/usr/bin/python3

import pathlib
from os.path import abspath

path = pathlib.Path(__file__).parent.absolute()
path = str(path) + "/.."

print(abspath(path))
