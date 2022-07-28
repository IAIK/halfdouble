# C4 - vFork

This PoC shows the architectural bit-flip verifiaction using vFork (see Section 6.4). 

This PoC consits of one part:
- [main.cpp](main.cpp)

## Building the PoC
Build the PoC with:

```
make
```

## PTEditor Optional

Please follow [PTEditor](../../README.md) to install the PTEditor kernel module. If the PoC is executed on the provided hardware, the pteditor kernel module is included in the custom kernel so running the following is sufficient to load the module:

```
modprobe pteditor
```

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

## Comment
There is no reason behind the 20th page. You can choose any page within the allocated range.

Bit 45 is one of the reserved bits on ARM system leading to the corruption exception, which kills an application (including the parent!) if the operating system detects such corruption. In contrast, flipping bit 15 will only corrupt the PFN of the Page Table Entry and redirect the reader to another page. However, this will not trigger a "corruption" exception and has no visible effect on the PoC. This is the reasoning behind choosing bit 45.

## Paper result:

From 6.4. Paragraph Evaluation (2nd):
- 19.45 GBps (n=10 , s_x= 0.039) on the Chromebook2
- 256.47 GBps (n=10, s_x=3.971 ) on the T490s