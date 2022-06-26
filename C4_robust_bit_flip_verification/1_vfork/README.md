# C4 - vFork

This PoC shows the architectural bit-flip verifiaction using vFork (see Section 6.4). 

This PoC consits of one part:
- [main.cpp](main.cpp)

## Building the PoC
Build the PoC with:

```
make
```

## Deploying the Artifacts

Please see [Deploying the Artifacts](../../README.md).

## Running the PoC
To execute the PoC on x86 run:

```
./c4_vfork_x86
```

To execute the PoC on aarch64 run:

```
./c4_vfork_aarch64 
```

To test the behavior of vfork when flipping one of the reserved bits, execute:

```
./c4_vfork_{x86|aarch64} 45
```

This will flip bit *45* in the 20th-page table entry.

### Expected Output
The expected output of this PoC:

```
Took 710921 ns to check 204804096 bytes = 268.297994 GBps
```