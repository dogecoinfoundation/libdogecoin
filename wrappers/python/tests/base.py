"""
cython libdogecoin testcase
"""

import unittest
import libdogecoin.libdogecoin as l

class TestCaseLibdogecoin(unittest.TestCase):
    def setUpClass():
        """
        libdogecoin context_start
        """
        l.context_start()

    def tearDownClass():
        """
        libdogecoin context_stop
        """
        l.context_stop()
