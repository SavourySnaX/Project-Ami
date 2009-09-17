---
layout: post
author: Savoury SnaX
title: Staged CPU 
---

 Not a lot of work been done over the last couple of weeks, however I have begun the work of staging the cpu instructions.

 All appears to be working pretty well so far, i have been testing cycle times between emulation and the real amiga and so far so good. I also fixed a few issues with the memory map (namely that chip ram is mirrored within the first 2Mb region - confirmed on my a500). 

 Also fixed up a problem with the vertical blank interrupt which was being incorrectly masked twice - which fixed a problem with one of my test disks. 

 I am still having dodgy key repeat issues, and i have made little attempt to fix the hardware chip emulation failings while I concentrate on the cpu. Hopefully the cpu won't take too much more time, although ideally i could do with building a 68000 circuit to fully test my staged emulation is accurate.

 The trouble is that with just an amiga it is nearly impossible to figure out on which cycles writes/reads to memory occur. For this i really need to measure it from the outputs of the cpu. A task for a much later date I think.
