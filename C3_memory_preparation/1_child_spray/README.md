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

## Running the PoC

To run the PoC on the Chromebooks, run:

```
./c3_child_spray_aarch64
```

### Expected output:

```
Sprayed 20446208 page tables (163569664 byte) in 1409607601 ns = 110.663554 MBps
```

## Paper result:

From 6.3. Paragraph Evaluation:
- 79.39 MBps (n=10, s_x=0.24) on Chromebook1
- 54.09 MBps (n=10, s_x=1.437) on Chromebook2
- 25.42 MBps (n=10, s_x=0.346) on the OnePlus 5T
- 99.88 MBps (n=10, s_x=0.456) on the Lenovo T490s