import doctest
import getopt
import glob
import sys

try:
    import pkg_resources

    pkg_resources.require("mapscript")
except (ImportError, pkg_resources.DistributionNotFound):
    pass


def run(pattern):
    if pattern is None:
        testfiles = glob.glob("*.txt")
    else:
        testfiles = glob.glob(pattern)
    for file in testfiles:
        doctest.testfile(file)


if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "t:v")
    except getopt.GetoptError:
        print("Usage: python runalldoctests.py [-t GLOB_PATTERN]")
        sys.exit(2)
    pattern = None
    for o, a in opts:
        if o == "-t":
            pattern = a
    run(pattern)
