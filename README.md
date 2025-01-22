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
[motor](https://github.com/opencog/motor).
As the names suggest, this new research effort attempts to take an
agent-centric view of the world: of an "inside" and an "outside",
with the "inside" being able to look out and explore and manipulate
the "outside" world. The agent works directly with
[Atomese](https://wiki.opencog.org/w/Atomese) to control what it
"sees", and to "move around" in the "external world". These words
are in quotes, because they are metaphorical: one can move around
through a file system, or 'surf the net' (moving from one website
to another). "Moving" doesn't have to involve literal arms and legs
and wheels (although it could. Physical robot control is one of the
goals.)

Similarly, one can "look at" a file in a file system, "look at" (listen
to?) a chat conversation, or "look at" a data stream flowing out of some
science experiment (Kamiokande neutrino detector, Vera Rubin telescope,
...) Again, "looking and seeing" does not need to be literally video
and audio.  Note that "looking" does have a motor aspect: the looker
does need to actively change the focus of attention in order to "look"
at something. Thus sensori- is coupled to -motor, unless you plan to
put your agent in prison and have them stare out all day.

The sensori-motor interface is not magic: to some large degree, its
old-fahsioned open, close, read, write. What makes it different is that
it is expressed in [Atomese](https://wiki.opencog.org/w/Atomese).
This is what provides the agent with deliberative, intentional
abilities and control. The agent gains autonomy over it's actions,
instead of being passively force-fed some data-stream outside of it's
control.

In principle, the sensori-motor system should someday be able to do
everything that the `StorageNode` does, and a lot more. For now,
however, it is an active research & development project, unstable,
incomplete and buggy.

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
              This includes a
              [ReadThruProxy](https://wiki.opencog.org/w/ReadThruProxy)
              and a
              [WriteThruProxy](https://wiki.opencog.org/w/WriteThruProxy),
              These pass storage I/O requests on to other StorageNodes.
              This is useful for configuring complex I/O pipelines
              in Atomese.

* sexpr    -- Read and write Atomese s-expressions as UTF-8 strings.
              Used both by the `FileStorageNode` and by `CogStorageNode`.
              The latter works in conjunction with the
              [CogServer](https://github.com/opencog/cogserver)
              to provide network-distributed AtomSpaces.

* sexcom   -- Respond to a very special set of 17 s-expression commands.
              These are used to provide high-speed network I/O for the
              CogServer, to provide network-distributed AtomSpaces.

* tlb      -- Implements a table that issues a unique integer ID for an
              Atom. Useful, if you think your code needs to substitute
              integer ID's for Atoms. Note that doing this consumes both
              RAM, to store the table, and CPU, to perform the lookup.
              So using it is almost surely a bad idea, in general. But
              if you think you really really want this, well, here it is.


Build and Install
-----------------
The module builds and installs just like all other AtomSpace modules.
There are no prerequistes, other than the AtomSpace itself. The general
instructions are:
```
cd /where/ever/you/cloned/this/to
rm -rf build
git checkout master
git pull
mkdir build
cd build
cmake ..
make -j
sudo make install
```
The unit tests can be run by saying `make check`. They should all pass.

Interesting Reading
-------------------
There are some interesting comments about (distributed) storage in the
"Big Graph Anti-Pattern" blog post: https://blog.blazegraph.com/?p=628

Atomese query most closely resembles the fourth bullet in that post:
"Graph query (aka graph pattern matching)"  Based on this, the
most promising backend would seem to be "blazegraph":
http://sourceforge.net/projects/bigdata/ (GPLv2)
http://www.blazegraph.com/

Please open new discussions, issues or pull requests if you think there
are other interesting things worth mentioning.
