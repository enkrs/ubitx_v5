# ubitx v5 firmware by YL3AME

Firmware for the version 5 of the ubitx.

This is my version of the v5 software for my ubitx radio build.
[Ubitx](https://www.hfsignals.com/index.php/ubitx-circuit-description/)
is a hackable 10W multi band HF radio that you can build for around
150$.

I'm using a tiny 128x64 OLED screen via i2c. Most of the buttons are
corrected to digital pins (vs analog in the original) leaving the analog
pins free for future measurements.

Code is refactored into namespaced C++ and trying to stick to google C++
style guide where possible for AVR. Care is taken to reduce code/memory
size.

I've implemented some variant of a non-blocking task scheduling loop,
allowing me to do lot's of background stuff more easy in future.

Menu logic has been refactored to hybrid of finite state machine / event
loop.

This is mostly an embedded c++ learning experience for me, but the radio
does work and I use this firmware.
