# C2 - Overall Flips

This PoC is used to record Table 1 of the paper. We use Half-Double to hammer memory both with uncachable memory and the flush instruction.

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

## Running the PoC

To run the PoC on the Chromebook, run:

```
./c2_overall_flips_aarch64
```

Note: This PoC will run in an endless loop and not terminate! To stop hit `Ctrl+c`.

### Expected output:

```
opened pagemap
address: 0x76317c6000
aligned address: 0x7631800000
got memory 0x7631800000
advised huge pages
mapped all pages 122703872
Offset: 0, BRC: 0
mem: 0x7631800000
physical address: f4200000
1ff000	34073	57	0x76319ff000	0xf43ff000	0 0 0
Hammer rows 0 4, 0x7631800000 0x7631822000, f4200, f4222FLIP FOUND! 57 != 55 0x763181128d
FLIP FOUND! 57 != 55 0x7631811291
Hammer rows 0 4, 0x7631800000 0x7631822000, f4200, f4222  took 1359357 us, 0 flips
tries: 2, UC flips 0->1: 2, 1->0: 0, FLUSH flips 0->1: 0, 1->0: 0
Hammer rows 1 5, 0x7631808000 0x763182a000, f4208, f422a  took 1361208 us, 0 flips
tries: 4, UC flips 0->1: 2, 1->0: 0, FLUSH flips 0->1: 0, 1->0: 0
Hammer rows 2 6, 0x7631811000 0x7631833000, f4211, f4233  took 1359224 us, 0 flips
tries: 6, UC flips 0->1: 2, 1->0: 0, FLUSH flips 0->1: 0, 1->0: 0
Hammer rows 3 7, 0x7631819000 0x763183b000, f4219, f423bFLIP FOUND! ba != aa 0x763182a86c
FLIP FOUND! ae != aa 0x763182a878
FLIP FOUND! ab != aa 0x763182a87c
  took 1360936 us, 3 flips
tries: 8, UC flips 0->1: 5, 1->0: 0, FLUSH flips 0->1: 0, 1->0: 0
Hammer rows 4 8, 0x7631822000 0x7631877000, f4222, f4277FLIP FOUND! ea != aa 0x7631833c8c
FLIP FOUND! ea != aa 0x7631833c98
FLIP FOUND! a8 != aa 0x7631833c9c
  took 1360968 us, 3 flips
...
```

## Paper result:
See Table 1.