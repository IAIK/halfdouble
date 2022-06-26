# C4 - Speculative Oracle

This PoC creates a valid mapping and an invalid one (containing an invalid PFN). Uses speculative execution to probe if the mapping is valid or not (see Section 6.4).

This PoC consits of one part:
- [main.cpp](main.cpp)

## Building the PoC
Build the PoC with:

```
make
```

The [Makefile](Makefile) builds both `aarch64` and `x86` versions.

## Deploying the Artifacts

Please see [Deploying the Artifacts](../../README.md).

## Running the PoC
To execute the PoC on x86 run:

```
./c4_spectre_x86
```

To execute the PoC on aarch64 run:

```
./c4_spectre_aarch64 
```

To test the behavior of vfork when flipping one of the reserved bits, execute:

```
./c4_spectre_{x86|aarch64} 45
```

This will flip bit *45* in the 20th-page table entry.

### Expected Output
The expected output of this PoC:

```

```
