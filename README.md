# CAGE: Cache-Aware Graphlet Enumeration

## Choosing the algorithm variant
Before compiling, you can choose which variant of CAGE to use.
To do this, open the `main.cpp` file and uncomment the macro corresponding to the version of the algorithm you want to use.
If no macro is uncommented, the code will default to CAGE (i.e., CAGE-3).

You can also uncomment the macros STATS_ENABLED to the see the median values of |N(S)|, |N(u)| and |N(z)|, or VTUNE to instruct the code for vtune profiling.
See the paper "CAGE: Cache-Aware Graphlet Enumeration" for more details on these values.

## Building
Make sure you have the `cmake` version >= 3.5 and `make` utilities installed.

After choosing the algorithm variant, issue these commands:
```bash
mkdir build
cd build
cmake ..
make -j CAGE
```

The executable file CAGE will be created in the `build` directory. 

## Usage

You can run CAGE using
```bash
./CAGE <path_to_input_graph> <k> [edge list format]
```

The last parameter is optional and if specified will cause the program to interpret the input file in the edge list format (see below).

## Graph Format
The main graph format used is as follows:
first row should contain the number of vertices n.
Then, n rows with `index degree` (separated by a space) with the degrees of each vertex.
Then the edge list of the graph, one edge per row `u v`.

If the third parameter is passed on the command line, then only the edge list may be supplied.
Vertex indices should start from 0.
