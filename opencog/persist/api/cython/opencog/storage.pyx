#
# This file reads files that are generated by the OPENCOG_ADD_ATOM_TYPES
# macro so they can be imported using:
#
# from storage import *
#
# This imports all the python wrappers for atom creation.
#
import warnings

from opencog.atomspace import types
from opencog.atomspace import regenerate_types
from opencog.utilities import add_node, add_link

regenerate_types()

include "opencog/persist/storage/storage_types.pyx"

def cog_open(stonode) :
	print("want to open ", stonode)

def cog_close(stonode) :
	print("want to close ", stonode)

