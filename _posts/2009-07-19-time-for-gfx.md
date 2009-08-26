---
layout: post
author: Savoury SnaX
title: Time for GFX
---

 So I decided I wanted to see if at least the kickstart rom is doing something useful. To that end I added SDL to the project and stuck in an emulation for the copper chip. I can see the grey scale cycling that occurs when an A500 boots up, thats a good sign.

 Unfortunately without blitter emulation I doubt I will see the friendly disk that shows up on a successful boot. I will have to refactor some of the code, I jammed the display decoding directly into the custom chip update.

* Added vbl interrupt handler
* Added crude 1 bpl display code to custom chip update.
* Added first pass copper emulation.

