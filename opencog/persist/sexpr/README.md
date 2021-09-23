Atomese S-Expressions
---------------------
Read and write UTF-8 Atomese s-expression strings. This is a collection
of utilities that take Atomese s-expressions and convert then into
in-RAM Atoms and Values living in an AtomSpace. It is intended to make
it easy and fast to serialize and deserialize AtomSpace contents, so
as to save Atomese in a file, or ship it over the network.

The code is meant to be "fast", because it is nearly an order of
magnitude (10x) faster than passing the same contents through the
scheme/guile interfaces. The speed is in fact roughy comparable to
working with Atoms directly, in that it takes about the same amount
of CPU time to string-compare, string-substring, string-increment,
as it does to create Atoms and place them in the AtomSpace. (Yes,
we measured. Results were a mostly-even 50-50 split.)

Only Atomese s-expressions, plus a very special subset of other
functions are supported. This is NOT a generic scheme interpreter.

The code includes a file-reader utility.  It also implements the
`FileStorageNode`, while implements some of the `StorageNode` API.
Entire AtomSpaces can be read and written. Individual Atoms can be
written. More complex queries offered by `StorageNode` are not
supported (because, obviously, flat files aren't databases. Use the
`RocksStorageNode` if you need a full-featured file-backed
`StorageNode`.)

C++ API
-------
The `fast_load.h` file defines the C++ API.

Python API
----------
There's a python wrapper for this, somewhere. Not sure where.

Scheme API
----------
The fast file loader loads Atomese formatted as scheme. So, of course,
you could just use the scheme `load` function!  But this can be slow,
painfully slow. The fast loader provides a 10x performance improvement.

Use it like so:
```
(use-modules (opencog))
(use-modules (opencog persist-file))

(load-file "/some/path/to/atomese.scm")
```

where `atomese.scm` contains Atomese. The contents of the AtomSpace can
be written out by saying `(export-all-atoms "/tmp/atomese.scm")`. The
`export-atoms`, `cog-prt-atomspace` and `prt-atom-list` are useful for
writing Atoms to a file.

* `export-atoms LST FILENAME` -- Export the atoms in LST to FILENAME.
* `prt-atom-list PORT LST`    -- Print a list LST of atoms to PORT.
  Prints the list of atoms LST to PORT, but only those atoms
  without an incoming set.
* `export-all-atoms FILENAME` -- Export entire atomspace into file

Example:
```
(define fp (open-file "/tmp/foo.scm" "w"))
(prt-atom-list fp (list (Concept "a")))
(close fp)
```

`StorageNode` API
-----------------
The `FileStorageNode` atom implements some of the `StorageNode` API.
I'ts just enough to save and load entire AtomSpaces, or to store
individual Atoms.  Here's a short example of writing selected Atoms,
and the entire AtomSpace, to a file. See also `persist-store.scm` in
the main examples directory.

```
(use-modules (opencog) (opencog persist) (opencog persist-file))

; Populate the AtomSpace with some data.
(define a (Concept "foo"))
(cog-set-value! a (Predicate "num") (FloatValue 1 2 3))
(cog-set-value! a (Predicate "str") (StringValue "p" "q" "r"))

(define li (Link (Concept "foo") (Concept "bar")))
(cog-set-value! li (Predicate "num") (FloatValue 4 5 6))
(cog-set-value! li (Predicate "str") (StringValue "x" "y" "z"))

; Store some individual Atoms, and then store everything.
(define fsn (FileStorageNode "/tmp/foo.scm"))
(cog-open fsn)
(store-atom a fsn)
(store-value a (Predicate "num") fsn)
(store-value a (Predicate "num") fsn)
(store-value a (Predicate "num") fsn)
(barrier fsn)

(store-atomspace fsn)  ; Store everything.
(cog-close fsn)
```

Here's an example of reading back what was stored above:
```
(use-modules (opencog) (opencog persist) (opencog persist-file))

(define fsn (FileStorageNode "/tmp/foo.scm"))
(cog-open fsn)
(load-atomspace fsn)
(cog-close fsn)

; Verify the load
(cog-prt-atomspace)

(define li (Link (Concept "foo") (Concept "bar")))
(cog-keys li)
(cog-value li (Predicate "num"))
(cog-value li (Predicate "str"))
```


Network API
-----------
The CogServer provides a network API to send/receive Atoms over the
internet. The actual API is that of the StorageNode (see the wiki page
https://wiki.opencog.org/w/StorageNode for details.) The cogserver
supports the full `StorageNode` API, and it uses the code in this
directory in order to make it fast.

To aid in performance, a very special set of about 15 scheme functions
have been hard-coded in C++. These are implemented in `Commands.cc`
The goal is to avoid the overhead of entry/exit into guile. This works
because the cogserver is guaranteed to send only these commands, and no
others.
