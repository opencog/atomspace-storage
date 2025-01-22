AtomSpace I/O: The StorageNode API
----------------------------------
This
[AtomSpace](https://github.com/opencog/atomspace)
module contains the base class, called the
[StorageNode](https://wiki.opencog.org/w/StorageNode)
for many/most forms of I/O storage and movement of
[Atoms](https://wiki.opencog.org/w/Atom) into, out of and between
AtomSpaces. It is mandatory for the
[RocksStorageNode](https://github.com/opencog/atomspace-rocks)
which stores Atoms and AtomSpaces to disk, via RocksDB, and to
[CogStorageNode](https://github.com/opencog/atomspace-cog), which
moves Atoms between AtomSpaces on different network nodes, and
[ProxyNodes](https://wiki.opencog.org/w/ProxyNode) which provides
mirroring, routing, caching and filtering between AtomSpaces.
An assortment of other interesting odds and ends can be found here.

### Alternatives
The `StorageNode` API is the original, primary API for moving Atoms
around. It's 'mature': full-featured, fully debugged, heavily used,
stable, complete.

Just like any mature API, it works great, but we wish it did some
things differently. Towards that end, a new I/O paradigm is being
actively researched, called
[sensory](https://github.com/opencog/sensory)
and
[motor](https://github.com/opencog/motor)
As the names suggest, this new research effort attempts to take an
agent-centric view of the world: of an "inside" and an "outside",
with the "inside" being able to look out explore and manipulate the
"outside" world. Data (Atoms and
[Values](https://wiki.opencog.org/w/Value) flow in and flow out,
with the "agent" "inside" being able to actively open, close, read,
write datastreams, instead of passively being force-fed data-streams
via some external pipeline or harness.

In principle, the sensori-motor system should someday be able to do
everything the `StorageNode` does, and a lot more. For now, however,
it is an active research & development project, unstable, incomplete
and buggy.

New subsystems that were "needed yesterday" and have to be absolutely
100% stable and rock-solid out the gate should be developed using the
StorageNode API. If you want to explore a weird and strange different
world, take a look at the sensori-motor system.

### Contents
The directory naming scheme here follows that in all other AtomSpace
projects. The main code is in [opencog/persist](opencog/persist).
Local subdirectories include:

* api      -- The generic StorageNode API.
              It provides open, close, load and store primitives that
              work for any of the standard I/O back-ends, including
              RocksDB and the CogServer.

* csv      -- Load Values from CSV/TSV files. These are "delimiter
              separated values" -- ordinary tables. Each column in the
              table is loaded into an appropriate Value (`FloatValue`,
              `BoolValue` or `StringValue`). The values are placed
              under keys (named after the column) on the provided Atom.
              This is intended for the ASMOSES subsystem, which
              naturally operates on tables or streams of data.

* file     -- Read and write files containing Atomese s-expressions.
              Provides both a `FileStorageNode`, and also some utilities
              to read files, and dump Atomspace contents to files or
              guile ports (without having to use `StorageNode`s.)

* flow     -- Implement the `FetchValueOfLink` and the `StoreValueOfLink`
              These allow specific Values to be obtained from storage,
              with Atomese. That is, it allows complex Atomese scripts
              to be written, that will work with Storage.

* json     -- Read and write Atomese JSON expressions. Suitable for
              web applications. Note, however, the `sexpr` interface
              is superior for performance and function.

* metta    -- Import and export fragments of MeTTa as Atomese. This
              provides only a fragment of MeTTa, currently consisting
              only of function declarations.

* prolog   -- Import and export fragments of prolog (datalog) as
              Atomese. This presumes only the simplest, most basic
              mapping: `:- siblings(jack, jill).` becomes
              `(Evaluation (Predicate "siblings") (List (Concept "jack") (Concept "jill")))`
              This is just enough to encode prolog facts and Horn
              clauses as Atomese.

* proxy    -- Implements a hierarchy of StorageNodes that act as agents.
              This includes a ReadThruProxy and a WriteThruProxy, that
              will pass storage I/O requests on to other StorageNodes.
              This is useful for configuring complex I/O pipelines
              in Atomese.

* sexpr    -- Read and write Atomese s-expressions as UTF-8 strings.
              Used both by the `FileStorageNode` and by `CogStorageNode`,
              which works in conjunction with the CogServer to provide
              network-distributed AtomSpaces.

* sexcom   -- Respond to a very special set of 17 s-expression commands.
              These are used to provide high-speed network I/O for the
              CogServer to provide network-distributed AtomSpaces.

* tlb      -- Implements a table that issues a unique integer ID for an
              Atom. Useful, if you think your code needs to substitute
              integer ID's for Atoms. Note that doing this consumes both
              RAM, to store the table, and CPU, to perform lookup. So it
              is probably not a good idea, in general. But if you really
              really need this, well, here it is.


Interesting Reading
-------------------
There are some interesting comments about (distributed) storage in the
"Big Graph Anti-Pattern" blog post: https://blog.blazegraph.com/?p=628

Atomese query most closely resembles the fourth bullet in that post:
"Graph query (aka graph pattern matching)"  Based on this, the
most promising backend would seem to be "blazegraph":
http://sourceforge.net/projects/bigdata/ (GPLv2)
http://www.blazegraph.com/

Future directions
-----------------
The `StorageNode` user API is implemented in about a dozen different
scheme (and python) functions.  This is great, if you are a human
writing ordinary code. Not so hot, if you're an algorithm trying to
get things done.  Therefore, it is anticipated that an API based on
the [sensori-motor system](https://github.com/opencog/sensory) will
be provided at some point in the future (i.e. when it becomes urgent
to do so.)

In the sensori-motor system all things external to the AtomSpace are
described by Atomese, and are accessible via pure Atomese. The goal is
to open up the external world to algorithms.
