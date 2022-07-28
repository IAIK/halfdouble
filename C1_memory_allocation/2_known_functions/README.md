# C1 - Via Known Functions

This PoC is used in Section 6.1 of the paper. We use the known DRAM addressing functions of the Chromebooks to build a contiguous memory detector.

This PoC consits of two parts:
- [simulate_patterns.py](simulate_patterns.py)
- [main.cpp](main.cpp)

## Simulating the Patterns
First, we include a tool to generate all the bank access patterns.

Run the following command to simulate the patterns:
```
python3 simulate_patterns.py 
```

The expected output should look as follows:

```
----------------------------------------------------------------------
Bank | d0  d1  d2  d3  d4  d5  d6  d7  d8  d9  d10 d11 d12 d13 d14 d15
----------------------------------------------------------------------
   0 |   8   9   8   9   8   9   8   9   8   9   8   9   8   9   8   1
   2 |   8   7   8  11   8   7   8  11   8   7   8  11   8   7   8   3
   4 |   8   9   8   5   8   9   8  13   8   9   8   5   8   9   8   5
   6 |   8   7   8   7   8   7   8  15   8   7   8   7   8   7   8   7
   8 |   8   9   8   9   8   9   8   1   8   9   8   9   8   9   8   9
  10 |   8   7   8  11   8   7   8   3   8   7   8  11   8   7   8  11
  12 |   8   9   8   5   8   9   8   5   8   9   8   5   8   9   8  13
  14 |   8   7   8   7   8   7   8   7   8   7   8   7   8   7   8  15
----------------------------------------------------------------------
```

These patterns are used in [main.cpp](main.cpp) to detect the contiguous memory regions using a timing side channel.

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

The [Makefile](Makefile) builds both `aarch64` and `x86` versions. However, the PoC uses the Chromebooks patterns, so the correctness is only given on the actual hardware. However, the `x86` exploit's performance is comparable as the timing side channel is executed correctly. The overhead to detect and store the Chunks is negligible in comparison to the timing side channel.

## Running the PoC

The PoC uses the `row_conflict_threshold` (see [U1 - Row Conflict Threshold](../../utility/1_row_conflict_threshold)) as input, i.e., the timing threshold between same-bank access and cross-bank access in ns.

To run the PoC on the Chromebook2, run:

```
./c1_known_functions_aarch64 315
```

### Expected output:

```
running in DEBUG!
100.0%
...
Chunk: 0x73842ae000 + [  604.0 kB] -> c
Chunk: 0x7384354000 + [  692.0 kB] -> c
Chunk: 0x7384410000 + [  964.0 kB] -> c
Chunk: 0x7384563000 + [  660.0 kB] -> c
Chunk: 0x738461f000 + [  812.0 kB] -> c
Chunk: 0x738475b000 + [ 1176.0 kB] -> c
Chunk: 0x7384890000 + [ 3284.0 kB] -> X
Chunk: 0x7384bd4000 + [ 1084.0 kB] -> c
Chunk: 0x7384d63000 + [ 1204.0 kB] -> c
Took 30374069617 ns to check 409600000 bytes = 0.028912 GBps
```

## Paper result:
From 6.1. Paragraph Evaluation:
- less than 3 minutes runtime
- 19.05 MBps (n=10, s_x=0.002) on Chromebook1
- 13.03 MBps (n=10, s_x=0.003) on Chromebook2
- 18.39 MBps (n=10, s_x=0.006) on the OnePlus 5T
- 46.55 MBps (n=10, s_x=0.455) on the T490s