# C2 - Overall Flips

This PoC shows the ECC-aware templating methode. We use Half-Double to hammer memory both with uncachable memory and the flush instruction, when we place *fake* PTEs in the victims location (see Section 6.2).

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
./c2_ecc_aware_aarch64
```

### Expected output:

```
...
Hammer rows 43 47, 0x75e496e000 0x75e494c000, 111f6e, 111f4c  took 2697578 us, 0 flips
tries: 88, UC flips 0->1: 6, 1->0: 1, FLUSH flips 0->1: 0, 1->0: 0
Hammer rows 44 48, 0x75e4955000 0x75e4980000, 111f55, 111f80FLIP FOUND! 2068000555555fd3 != 68000555555fd3 0x75e49448e0
FLIP FOUND! 2068000555555fd3 != 68000555555fd3 0x75e49448e8
FLIP FOUND! 68000575555fd3 != 68000555555fd3 0x75e49448f0
FLIP FOUND! 68000575555fd3 != 68000555555fd3 0x75e49448f8
Hammer rows 44 48, 0x75e4955000 0x75e4980000, 111f55, 111f80  took 2695629 us, 0 flips
...
```

