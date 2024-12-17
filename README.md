# phnsw: An optimized HNSW (Hierarchical Navigable Small World graphs) ASIP model for SST Simulator
## Getting Started
- This project is under development. It is compeletly unfunctional currently (now DMA is a little functional).

## Prerequisites
Make sure your [SST Core](https://github.com/sstsimulator/sst-core) and [SST Elements](https://github.com/sstsimulator/sst-elements) is installed successfully.

## Building
This phnsw component is derived from [sst-external-element](https://github.com/sstsimulator/sst-external-element) (An example for u to create your own SST Componets) and it is portable to SST.
That means you can build this componet individually rather than building the whole [sst-elements](https://github.com/sstsimulator/sst-elements).

The building scripts in [Makefile](./src/Makefile) will register this component automatically after building. All you need to do is to execute this in your command line:
```
make
```
Then you can run the simulation by typing:
```
sst tests/phnsw-test-001.py
```
