J2ME CLDC 1.1 / MIDP 2.0 emulator for Windows
==============================================

What this is
------------
midp.exe is a 32-bit Windows build of the KVM-based J2ME virtual machine
(CLDConfig 1.1 + MIDP 2.0) from the j2me_cldc source tree.

* 32-bit PE executable (machine type i386 / 0x14C).
* Runs natively on 32-bit Windows, and on 64-bit Windows via WoW64
  (which is enabled by default on all modern x64 Windows versions).
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
* The VM heap default is ~1 MB; raise it with -heapsize if a suite needs
  more (must be divisible by 4, e.g. -heapsize 2000000).
