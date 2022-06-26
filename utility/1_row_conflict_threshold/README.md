# U1 - Row Conflict Threshold

This utility tool is used to get an estiamte for the row conflict threshold on the Chromebooks.

This tool consits of one part:
- [main.cpp](main.cpp)

## Building the PoC

To build the tool run the following:

```
make 
```

The [Makefile](Makefile) builds both `aarch64` and `x86` versions. The tool only works correctly on the Chromebooks.

## Running the PoC

To run the PoC on the Chromebooks, run:

```
./u1_row_conflict_threshold_aarch64
```

Expected output:

```
 9999/10000
same  bank: 311.782280 ns
cross bank: 298.540355 ns
```

## Selecting the Threshold

For the above example a resulting threshold could be `305` for the `row_conflict_side_channel`.