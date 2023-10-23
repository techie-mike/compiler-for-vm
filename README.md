# JIT/AOT compiler for virtual machine
This is the repository of MIPT course where will be written a compiler with **Sea of nodes IR** for a virtual machine which is written earlier.

### Build project:
```bash
mkdir build && cd build
cmake -GNinja ..
ninja
```

### Run tests:
```bash
cd build
ninja tests
```

### Source list

1) `A Simple Graph-Based Intermediate Representation`, Cliff Click, Michael Paleczny, 1995
2) `Global code motion/global value numbering`, Cliff Click, 1995
