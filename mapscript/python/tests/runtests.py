
import os, sys
import distutils.util
import unittest

# Construct the distutils build path, allowing us to run unit tests 
# before the module is installed
platformdir = '-'.join((distutils.util.get_platform(), 
                       '.'.join(map(str, sys.version_info[0:2]))))
sys.path.insert(0, 'build/lib.' + platformdir)



from hashtest import HashTableTestCase
from testMapScript import LayerTestCase

suite = unittest.TestSuite()
suite.addTest(HashTableTestCase)

if __name__ == '__main__':
    unittest.main()

