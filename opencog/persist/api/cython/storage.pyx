#
# This file reads files that are generated by the OPENCOG_ADD_ATOM_TYPES
# macro so they can be imported using:
#
# from storage import *
#
# This imports all the python wrappers for atom creation.
#
import warnings
from cython.operator cimport dereference as deref

from opencog.atomspace import types
from opencog.atomspace import regenerate_types
from opencog.utilities import add_node, add_link

from opencog.atomspace cimport cValuePtr, cHandle, cAtomSpace
from opencog.atomspace cimport Atom
from opencog.atomspace cimport create_python_value_from_c_value

# The list of Atom Types that python knows about has to be rebuilt,
# before much else can be done.
regenerate_types()

include "opencog/persist/storage/storage_types.pyx"

def cog_open(Atom stonode) :
	storage_open(deref(stonode.handle))

def cog_close(Atom stonode) :
	storage_close(deref(stonode.handle))

def cog_connected(Atom stonode) :
	return storage_connected(deref(stonode.handle))

# Utility, used below.
cdef pfromh(cHandle result) :
	if result == result.UNDEFINED: return None
	atom = create_python_value_from_c_value(<cValuePtr&>result)
	return atom

def fetch_atom(Atom atm) :
	return pfromh (dflt_fetch_atom(deref(atm.handle)))

def store_atom(Atom atm) :
	return pfromh (dflt_store_atom(deref(atm.handle)))

def fetch_incoming_set(Atom atm) :
	return pfromh (dflt_fetch_incoming_set(deref(atm.handle)))

def fetch_incoming_by_type(Atom atm, Type t) :
	return pfromh (dflt_fetch_incoming_by_type(deref(atm.handle), t))

def load_atomspace() :
	cdef cHandle zilch  # cHandle.UNDEFINED
	dflt_load_atomspace(zilch)

def store_atomspace() :
	cdef cHandle zilch  # cHandle.UNDEFINED
	dflt_store_atomspace(zilch)

def delete (Atom atm) :
	return dflt_delete(deref(atm.handle))

def delete_recursive (Atom atm) :
	return dflt_delete_recursive(deref(atm.handle))

def barrier() :
	return dflt_barrier()

def erase() :
	return dflt_erase()

def load_type() :
	return None

def fetch_value() :
	return None

def store_value() :
	return None

def update_value() :
	return None

def fetch_query() :
	return None

def proxy_open() :
	dflt_proxy_open()
	return None

def proxy_close() :
	dflt_proxy_close()
	return None

def monitor() :
	return dflt_monitor()

def curr_storage() :
	return pfromh (current_storage())


# -----------------------------------------
# Unrelated stuff

from opencog.atomspace cimport AtomSpace
# from opencog.storage cimport c_load_file

def load_file(path, AtomSpace atomspace):
	cdef string p = path.encode('utf-8')
	c_load_file(p, deref(atomspace.atomspace))

# -----------------------------------------
