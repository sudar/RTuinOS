What's new in Release R2?

Makefile revised. Both operating systems, Windows and Linus, are now
supported.
  Path conventions obeyed: Usage of forward slashes and trailing slash in
path names.
  Tools addressed by absolute paths to avoid conflicts with improperly set
Windows PATH variable.
  Build of different test cases decoupled; each now has its own build
folder. A clean is no longer necessary when switching the application.
  The creation of required working directories has been integrated into
the build. Directory creation is no longer a build rule, which has to be
called explicitly.
  The makefile has been split in parts. The configuration part is now
separated and clearly recognizable and readable to the user. The invariant
parts of the makefile have been hidden in a sub-directory.
  A kind of "callback" is made into the application folder. An (optional)
makefile fragment located here will be included into the build and permits
to override general settings in an application related fashion.

Support of Arduino 1.0.5, the current release as of today (29.5.2013)
  All test cases build and run with Arduino 1.0.5.

Support of mutexes and semaphores. The existing concept of events has been
extended. An event can now be of kind ordinary (broadcasted event, as
before), mutex or semaphore. Task resume conditions can continue to
combine any events regardless of the kind.

More assertions have been placed in the kernel for debug compilation,
which signal possible application errors, like an idle task, which tries
to suspend.

Minor, insignificant optimizations of the kernel code.

The doxygen documentation now includes some of the test cases, which
contain sample code of general interest.
