# coding: utf-8
import os
import sys

# Put the pymod dir on the path, so modules can `import pmstestlib`
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "pymod"))

# These files may be non-importable, and don't contain tests anyway.
# So we skip searching them during test collection.
collect_ignore = []
