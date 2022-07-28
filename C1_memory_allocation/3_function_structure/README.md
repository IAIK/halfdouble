# C1 - Function Structure

This PoC is used in Section 6.1 and Appendix C of the paper. We use the xor structure of the DRAM addressing functions to build a contiguous memory.

This PoC contains two parts:
- [solver.py](solver.py)
- [eval.py](eval.py)

## Running the PoC

To run the PoC, run:

```
python3 eval.py
```

### Expected output:

```
    device;    slength;     fscore;       MBps
       op5;          8;       0.59;       0.56
       op5;         16;       0.78;       0.36
       op5;         24;       0.81;        0.8
       op5;         48;       0.84;        1.6
       op5;         64;       0.95;        1.3
       op5;         96;       0.77;        2.2
       op5;        128;       0.83;        2.6
chromebook;          8;       0.52;       0.65
chromebook;         16;       0.75;        0.2
chromebook;         24;       0.89;       0.26
chromebook;         48;        1.0;        0.5
chromebook;         64;       0.98;       0.67
chromebook;         96;       0.96;        1.1
chromebook;        128;        1.0;        1.6
       px3;          8;       0.66;       0.67
       px3;         16;       0.77;       0.39
       px3;         24;       0.92;        0.6
       px3;         48;       0.94;        1.3
       px3;         64;       0.95;        1.7
       px3;         96;        1.0;        2.1
       px3;        128;        1.0;        2.4
        s9;          8;       0.52;       0.72
        s9;         16;       0.55;       0.22
        s9;         24;       0.75;       0.11
        s9;         48;        1.0;       0.13
        s9;         64;        1.0;       0.19
        s9;         96;        1.0;       0.44
        s9;        128;        1.0;       0.84
```

## Paper result:   
From 6.1. Paragraph Evaluation:

- average F-score of 0.97 with pattern length of 64
- average scanning speed of 1.079 MBps
- Note: The MBps depend on the hardware running the simulation