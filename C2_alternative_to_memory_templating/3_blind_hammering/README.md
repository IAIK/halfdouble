# C2 - Overall Flips

This PoC shows Blind-Hammering. We use Half-Double to hammer memory both with uncachable memory and the flush instruction, when we place *random* PTEs in the victims location  (see Section 6.2).

This PoC consits of one part:
- [main.cpp](main.cpp)

## Building the PoC

To build the PoC run the following:

```
make
```

The [Makefile](Makefile) builds both `aarch64` and `x86` versions. The PoC requires huge pages and the pteditor module as on the Chromebooks and assumes DRAM addressing functions as on the Chromebooks.

## PTEditor Required

Please follow [PTEditor](../../README.md) to install the PTEditor kernel module. If the PoC is executed on the provided hardware, the pteditor kernel module is included in the custom kernel so running the following is sufficient to load the module:

```
modprobe pteditor
```


## Deploying the Artifacts

Please see [Deploying the Artifacts](../../README.md).

## Running the PoC

To run the PoC on the Chromebook, run:

```
./c2_blind_hammering_aarch64
```

### Expected output:

```
...
Hammer rows 17 21, 0x6f2a88a000 0x6f2a8a8000, 10928a, 1092a8  took 2697332 us, 0 flips
tries: 516, UC flips 0->1: 0, 1->0: 0, FLUSH flips 0->1: 0, 1->0: 0
Hammer rows 18 22, 0x6f2a893000 0x6f2a8b1000, 109293, 1092b1  took 2696958 us, 0 flips
tries: 518, UC flips 0->1: 0, 1->0: 0, FLUSH flips 0->1: 0, 1->0: 0
Hammer rows 19 23, 0x6f2a89b000 0x6f2a8b9000, 10929b, 1092b9  took 2693999 us, 0 flips
FLIP FOUND! 68020018a6dfd3 != 68000018a6ffd3 0x6f2a8a86e0

FLIP FOUND! 6800000730cfd3 != 6800000730efd3 0x6f2a8a86e8
  took 2695944 us, 2 flips
tries: 520, UC flips 0->1: 1, 1->0: 2, FLUSH flips 0->1: 0, 1->0: 0
Hammer rows 20 24, 0x6f2a8a0000 0x6f2a8f5000, 1092a0, 1092f5  took 2698671 us, 0 flips
tries: 522, UC flips 0->1: 1, 1->0: 2, FLUSH flips 0->1: 0, 1->0: 0
...
```

