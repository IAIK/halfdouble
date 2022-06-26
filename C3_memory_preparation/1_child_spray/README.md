# C3 - Child Spray

This PoC is used in Section 6.3 of the paper. We use fork to create child processes and map the same 

This PoC consits of one part:
- [main.cpp](main.cpp)

## Building the PoC

To build the PoC run the following:

```
make 
```
The [Makefile](Makefile) builds both `aarch64` and `x86` versions.

## Deploying the Artifacts

Please see [Deploying the Artifacts](../../README.md).

## Running the PoC

To run the PoC on the Chromebooks, run:

```
./c3_child_spray_aarch64
```

### Expected output:

```
Sprayed 20446208 page tables (163569664 byte) in 1409607601 ns = 110.663554 MBps
```

