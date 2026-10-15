J2ME CLDC 1.1 / MIDP 2.0 emulator for Windows
==============================================

What this is
------------
midp.exe is a 32-bit Windows build of the KVM-based J2ME virtual machine
(CLDConfig 1.1 + MIDP 2.0) from the j2me_cldc source tree.

* 32-bit PE executable (machine type i386 / 0x14C).
* Runs natively on 32-bit Windows, and on 64-bit (x64) Windows via
  WoW64, which is enabled by default on all modern Windows releases.
  This is the standard way 32-bit Windows applications ship for x64.
* Why not a native x64 (AMD64) binary? The KVM core is a 32-bit
  "cell"-based VM: kvm/VmCommon/h/global.h declares
      typedef unsigned long cell;  /* Must be 32 bits long! */
  and object references (heap pointers) are stored in these 32-bit
  cells - operand stack slots, instance fields, KNI handles
  (see pushStack() in global.h and KNI_SETHAND() in kni.c). In a
  true 64-bit process the heap pointers would be truncated to 32
  bits, silently corrupting memory. Making the VM 64-bit is a VM
  port, not a build flag. Hence: i386 PE + WoW64.
* Statically linked - no runtime DLLs or installers required.
* Built cross-compiled with the Zig C compiler (target x86-windows-gnu).

Layout
------
  midp.exe        the emulator
  lib/            MIDP_HOME content: base classes (lib/classes),
                  system.config, internal.config, UI resources
  games/          example MIDlets shipped with the source tree
  appdb/          persistent storage (records / app DB) - created &
                  updated at runtime

MIDP_HOME is found automatically: the emulator locates "lib" next to
midp.exe (searching up to 3 levels), or honors the MIDP_HOME environment
variable if it is set.

Usage
-----
  midp.exe games\games.jar
      Run the example MIDlet suite. The MIDlet with the smallest
      MIDlet-N index is started (TilePuzzle for games.jar).

  midp.exe games\games.jad
      Same, via the JAD descriptor (uses MIDlet-JAR-URL from the JAD).

  midp.exe <path\to\your-suit.jar or .jad>
      Run any CLDC 1.1 / MIDP 2.0 MIDlet suite you have. The suite jar
      is added to the class path and the first MIDlet is started.

  midp.exe
      No argument: a file picker opens (default folder: <exe>\games)
      so you can pick a .jar/.jad. Cancelling drops you into the
      built-in manager (installed suites).

  midp.exe com.example.MyMidlet
      Run a specific MIDlet class from the base class path
      (must be a class already visible to the VM).

Other supported options (standard MIDP/KVM):
  -help, -version, -heapsize <N>, -classpath <dirs>
  -install <jad>  -uninstall <name>  -autotest <jad>  -transient <jad>
  -domain maximum|medium|minimum

Notes
-----
* Audio uses the Windows mixer (winmm); the screen renders to a standard
  top-level window with the emulator's LCD look.
* If a suite jar contains several MIDlets, the one with the smallest
  MIDlet-N number is selected automatically.
* The VM heap defaults to 1 MB (MAXIMUMHEAPSIZE). Raise or lower it
  with -heapsize (must be divisible by 4, e.g. -heapsize 2000000).
* Debug logging: all console output (C runtime + Java System.out/err)
  is also written to midp-debug.log in the current working directory,
  including a startup banner (midp_home, command, MIDlet class, JAD
  descriptor, classpath, heap) and the VM exit status. If the emulator
  closes unexpectedly, send that file for diagnosis.
