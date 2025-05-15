# phnsw: An optimized HNSW (Hierarchical Navigable Small World graphs) ASIP model for SST Simulator
## Getting Started
This proj is made up of two main parts:
1. phnsw core.
   - src/phnsw.h
   - src/phnsw.cc
2. phnswDMA.
   - src/phnswDMA.h
   - src/phnswDMA.cc

## Prerequisites
Make sure your [SST Core](https://github.com/sstsimulator/sst-core) and [SST Elements](https://github.com/sstsimulator/sst-elements) is installed successfully.

## Building
This phnsw component is derived from [sst-external-element](https://github.com/sstsimulator/sst-external-element) (An example for u to create your own SST Componets) and it is portable to SST.
That means you can build this componet individually rather than building the whole [sst-elements](https://github.com/sstsimulator/sst-elements).

The building scripts in [Makefile](./src/Makefile) will register this component automatically after building.

To build, run this:
```bash
$ cd src
$ make
```

## Run the simulation
To simulate, run this:
```bash
$ cd src
$ sst tests/phnsw-test-001.py
```

**OR** using simulate script:
```bash
$ cd src
$ bash run.sh
```
Note that this script will auto-build and run the simulation for 99 times. Each time with different enter point(ep). To achieve this, the script will modified the [instructions.asm](src/instructions/instructions.asm) to change the ep each time.

After 99 runs, an average time and Maximum time together with a list of time for each run will be printed.