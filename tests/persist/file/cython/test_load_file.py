import os
import random
import tempfile
import filecmp

from unittest import TestCase

from opencog.type_constructors import *
from opencog.atomspace import AtomSpace
from opencog.utilities import initialize_opencog, finalize_opencog, load_file

__author__ = 'Curtis Faith'


def write_sorted_file(path, atomspace):
    with open(path, 'wt') as f:
        for atom in sorted(list(atomspace)):
            f.write(str(atom))
            f.write('\n')


class UtilitiesTest(TestCase):

    def setUp(self):
        self.atomspace = AtomSpace()
 
    def tearDown(self):
        del self.atomspace

    def test_initialize_finalize(self):
        initialize_opencog(self.atomspace)
        finalize_opencog()

    def test_fast_load(self):
        gen_atoms(self.atomspace)
        with tempfile.TemporaryDirectory() as tmpdirname:
            tmp_file = os.path.join(tmpdirname, 'tmp.scm')
            write_sorted_file(tmp_file, self.atomspace)
            new_space = AtomSpace()
            load_file(tmp_file, new_space)
            self.assertTrue(len(new_space) == len(self.atomspace))
            # files should be binary equal
            new_tmp = os.path.join(tmpdirname, 'tmp1.scm')
            write_sorted_file(new_tmp, new_space)
            self.assertTrue(filecmp.cmp(tmp_file, new_tmp, shallow=False), "files are not equal")
            checklist = """(ListLink(ConceptNode "vfjv\\"jnvfé")
                (ConceptNode "conceptIR~~gF\\",KV")
                (ConceptNode "вверху плыли редкие облачка"))"""
            with open(tmp_file, 'wt') as f:
                f.write(checklist)
            new_space1 = AtomSpace()
            load_file(tmp_file, new_space1)
            self.assertTrue(len(new_space1) == 4)
