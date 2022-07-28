# Half-Double

This is the PoC implementation for the USENIX 2022 paper [**Half-Double: Hammering From the Next Row Over**](https://andreaskogler.com/papers/halfdouble.pdf) by [Andreas Kogler](https://andreaskogler.com), [Jonas Juffinger](https://twitter.com/notimaginary_), Salman Qazi, Yoongu Kim, [Moritz Lipp](https://mlq.me/), Nicolas Boichat, Eric Shiu, Mattias Nissler, and [Daniel Gruss](https://gruss.cc).


# Preparations

## Repository Initialization
After cloning this repository, use the following command to initialize the submodules.

```
make submodules
```

## PTEditor
For page table modifications, we use the [PTEditor](https://github.com/misc0110/PTEditor). The kernel module needed for the experiments is built with:

```
make pteditor
```

### Running on the Provided Hardware
If the PoCs are run on the provided hardware, the PTEditor module is precompiled with the kernel. Therefore, executing the following command is enough:

```
modprobe pteditor
```

## Ubuntu Packages

To build the PoC for both Arm and x86, we need to install a cross-compiler and additional tools (we assume the host is x86):

```
sudo apt install cmake ninja-build build-essential git python3 python3-pip g++-aarch64-linux-gnu gcc-aarch64-linux-gnu python3 
sudo apt install linux-tools-$(uname -r)
```

## Python packages
Please install the required python3 packages required via:

```
python3 -m pip install numpy z3-solver sklearn
```

# The Artficats
Please follow each README.md in the subfolders:

## Utility
- [Calibration Tool](./utility/1_row_conflict_threshold)

## C1 - Memory Allocation
- [Huge Pages](./C1_memory_allocation/1_huge_pages)
- [Known DRAM Functions](./C1_memory_allocation/2_known_functions)
- [DRAM Function Structure](./C1_memory_allocation/3_function_structure)

## C2 - Alternative to Memory Templating
- [Overall Flips](./C2_alternative_to_memory_templating/1_overall_flips)
- [ECC-Aware Templating](./C2_alternative_to_memory_templating/2_ecc_aware)
- [Blind Hammering](./C2_alternative_to_memory_templating/3_blind_hammering)

## C3 - Memory Preparation
- [Child Spray](./C3_memory_preparation/1_child_spray)

## C4 - Robust Bit-Flip Verification
- [vFork](./C4_robust_bit_flip_verification/1_vfork)
- [Speculative Oracle](./C4_robust_bit_flip_verification/2_speculative_oracle)

## End-to-End Exploit
- [Exploit](./end_to_end_exploit)


# Warnings
**Warning #1**: We are providing this code as-is. You are responsible for protecting yourself, your property and data, and others from any risks caused by this code. This code may cause unexpected and undesirable behavior to occur on your machine.

**Warning #2**: This code is only for testing purposes. Do not run it on any production systems. Do not run it on any system that might be used by another person or entity.
