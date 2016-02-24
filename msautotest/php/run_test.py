#!/usr/bin/env python

import sys
import subprocess

retcode = subprocess.call(['phpunit','.'])

sys.exit(retcode)
