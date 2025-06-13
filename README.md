# Algorithm Engineering Group Heidelberg - PACE 2025

Submission for PACE2025 by the algorithm engineering group at the Heidelberg University.
By Adil Chhabra, Marlon Dittes, Ernestine Großmann, Kenneth Langedal, Henrik Reinstädtler, Christian Schulz, Darren Strash, and Henning Woydt.

## Dependencies

* UWrMaxSat (only for exact solvers)
* Data reductions for the MWIS problem

The file **scipoptsuite-9.2.1.tgz** must be in the repository before any of the build scripts are executed. It can be downloaded [here](https://scipopt.org/index.php#download). Then, build UWrMaxSat by running the script

```
./get_uwrmaxsat_dep.sh
```

For the data reductions, the **get_dep.sh** script clones and builds the necessary library.

```
./get_dep.sh
```

## Build

Once the dependencies are built, simply run make to build both the exact and heuristic solvers.
```
make
```

## Running the programs

After building, there will be two executables, **EXACT** and **HEURISTIC**. They both take input and output from **stdin** and **stdout**. For example

```
./EXACT < input.gr > output.ds
```
Or
```
./HEURISTIC < input.gr > output.ds
```

Both solvers are used for the dominating set and hitting set problems. The internal parser reads the problem descriptor and the output will be a valid dominating set or hitting set depending on the input.