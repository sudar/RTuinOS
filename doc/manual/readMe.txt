The file manual.pdf is the main documentation of RTuinOS. You definitely
need to read it before starting with RTuinOS.

The manual of RTuinOS is open source. To build it from the source files,
the build environment (GNU make tool) needs to be set up as described for
RTuinOS itself in the manual.

The manual is written in LaTeX. The applied TeX compiler is MikTeX
(MiKTeX-pdfTeX 2.9.3962 (1.40.11) (MiKTeX 2.9)). This tool needs to be
installed; pdflatex.exe needs to be found on the Windows search path.

Once the environment is set up you just need to open a Command Prompt (or
Powershell), cd here into directory manual and type:

make

To get more information type

make help
