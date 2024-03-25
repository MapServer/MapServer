#!/usr/bin/env python

import subprocess
import sys

retcode = subprocess.call(["phpunit", "."])

sys.exit(retcode)
