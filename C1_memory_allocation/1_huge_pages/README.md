# C1 - Huge Pages

This PoC is used in Section 6.1 of the paper. We use the huge pages to *find* contiguous memory.

This PoC consists of one part:
- [main.cpp](main.cpp)

## Building the PoC

The PoC features both release and debug versions. The debug version verifies with privileged interfaces if the extracted chunks are contiguous.

Release version:

```
make 
```

Debug version:

```
make DEBUG=1
```

The [Makefile](Makefile) builds both `aarch64` and `x86` versions. The PoC requires huge pages as on the Chromebooks.

## Running the PoC

To run the PoC on the Chromebooks, run:

```
./c1_huge_pages_aarch64
```

### Expected output:

```
running in DEBUG!
...
Chunk: 0x760d800000 + [  2048.0 kB] -> c
Chunk: 0x760da00000 + [  2048.0 kB] -> c
Chunk: 0x760dc00000 + [  2048.0 kB] -> c
Chunk: 0x760de00000 + [  2048.0 kB] -> c
Chunk: 0x760e000000 + [  2048.0 kB] -> c
Chunk: 0x760e200000 + [  2048.0 kB] -> c
Chunk: 0x760e400000 + [  2048.0 kB] -> c
Chunk: 0x760e600000 + [  2048.0 kB] -> c
Chunk: 0x760e800000 + [  2048.0 kB] -> c
Chunk: 0x760ea00000 + [  2048.0 kB] -> c
Chunk: 0x760ec00000 + [  2048.0 kB] -> c
Took 20000 ns to check 409600000 bytes = 19073.486328 GBps
```

## Paper result:
From 6.1. Paragraph Evaluation:
- less than 10 seconds runtime
