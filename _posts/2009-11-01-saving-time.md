---
layout: post
author: Savoury SnaX
title: Saving Time
---

 I added save/load of hardware state today. This means i can let the emulation run for as long as needed, and then reload exactly that point in emulation.

 I spent much of the day debugging the blitter. Basically there has been something wrong with line drawing since i got work bench working. Classic example of this, was the single pixel error on the boot up image. Well it turned out to be a problem with the shifter on the B channel of the blitter. This is used to provide textured line support. However the first write of the line would potentially be masked out because i was initialising the previous shifted out data to zero. It seems more sensible to copy the data on line start into the previous shift register and this fixes the problem.
