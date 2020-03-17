# Xeros Kernel Implementation

This directory contains the source code for the Xeros operating system
which is a desicated version of the Xinu OS.

To build a Xeros image, type make in this directory. If you want to
launch bochs immediately after the make completes (assuming it is
successful) type "make beros" instead. This command will first build a
Xeros image and then start the bochs emulator.  (You can also go to
the c directory, where the source code you are editing resides, and
run the same make commands there.)

When you run "make", or "make xeros", the first two steps below are
performed. If you run "make beros" then all 3 steps are done.

1. Change to the compile directory and run make

2. If step 1 succeeds, a boot image is built by changing to the boot
directory and running make there. 

3) If step 3 succeeds, bochs is run.

If you simply type make you can, assuming there was a clean make, run
the resulting image by executing the bochs command in this directory
(i.e.  nice bochs)

Once bochs is running, choose option 6 to start the simulation and to
load and run the created image.


