What's new in Release R2?

Makefile revised. Path conventions obeyed: Usage of forward slashes and
trailing slash in path names.
  Tools addressed by absolute paths to avoid conflicts with improperly set
Windows PATH variable.
  Build of different test cases decoupled; each now has its own build
folder. A clean is no longer necessary when switching the application.
  The creation of required working directories has been improved.
Directory creation is no longer a build rule which has to be called
explicitly.

Support of Arduino 1.0.5, the current release as of today (29.5.2013)
  All test cases build and run with Arduino 1.0.5.