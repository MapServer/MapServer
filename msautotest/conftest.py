# coding: utf-8
import os
import sys

import pytest

# Put the pymod dir on the path, so modules can `import pmstestlib`
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "pymod"))

# These files may be non-importable, and don't contain tests anyway.
# So we skip searching them during test collection.
collect_ignore = ["pymod/msautotest_viewer.py"]


def pytest_addoption(parser):
    parser.addoption(
        "--strict_mode",
        action="store_true",
        default=False,
        help="Strict mode",
    )

    parser.addoption(
        "--valgrind",
        action="store_true",
        default=False,
        help="Run under valgrind",
    )

    parser.addoption(
        "--run_under_asan",
        action="store_true",
        default=False,
        help="Run under ASAN",
    )

    parser.addoption("--renderer", default=None)


@pytest.fixture
def extra_args(request):
    return request.config
