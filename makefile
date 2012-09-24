# 
# Generic Makefile for Arduino Project
#
# Compilation and linkage of C(++) code into binary files and download to the controller.
#
# The makefile is intended to be executed only by the Opus Make utility.
#   Help on the syntax of this makefile is got at http://www.opussoftware.com/index.htm,
# and particularly at http://www.opussoftware.com/tutorial/tutorial.htm and
# http://www.opussoftware.com/quickref/quickref.htm
#
# Preconditions
# =============
# 
# The name of the project needs to be assigned to the makefile macro projectName, see
# heading part of the code section of this makefile.
#   The makefile hard codes the hardware target. Among more, see makefile macros
# targetMicroController, cFlag and lFlags. Here, you will have to make some changes
# according to your selection of an ATmega micro controller. More changes will be required
# to the command lines of the object file tool avr-objcopy and the flash tool avrdude.
#   Hint: To find out how to run these tools, you can enable verbose mode in the Arduino
# IDE and click download for one of the code examples. Copy the contents of the IDE's
# output window and paste into a text editor. You will find appropriate command lines for
# all the tools.
#   The Arduino installation directory needs to be referenced. The location is determined
# by environment variable ARDUINO_HOME. The variable holds the name of the folder which
# arduino.exe is located in. Caution: This variable is not created by the original Arduino
# installation process but needs to be created manually.
#   The Windows path needs to contain the location of the GNU compiler/linker etc. This is
# the folder containing e.g. avr-gcc.exe down in the Arduino installation. Typically this
# can be derived from the environment variable as $(ARDUINO_HOME)\hardware\tools\avr\bin,
# but we want to have these tools directly available.
#
# Targets
# =======
#
# The makefile provides several targets, which can be combined on the command line. Get
# some help on the available targets by invoking the makefile using
#   make help
#
# Options
# =======
#
# Options may be passed on the command line.
#   The follow options may be used:
#   CONFIGURATION: The compile configuration is one out of DEBUG (default) or PRODUCTION. By
# means of defining or undefining macros for the C compiler, different code configurations
# can be produced. Please refer to the comments below to get an explanation of the meaning
# of the supported configurations and which according #defines have to be used in the C
# source code files.
#   COM_PORT: The communication port to be used by the flash tool needs to be known. The
# default may be adjusted to your environment in the heading part of the code section of
# this makefile or you may override the variable setting on the make processor's command
# line by writing e.g. make COM_PORT=\\.\COM3.
#
# Input Files
# ===========
#
# The makefile compiles and links all source files which are located in a given list of
# source directories. The list of directories is hard coded in the makefile, please look
# for the setting of srcDirList below.
#   Additionally required Arduino library files are hard coded in this makefile. They are
# referenced by absolute paths into the Arduino installation directory. Therefore, the
# installation directory needs to be known by means of an environment variable called
# ARDUINO_HOME.
#   These settings are invariant throughout the project life time and don't need
# maintenance.
#
# Copyright (c) 2012, Peter Vranken, Germany.
#
# All rights reserved. Reproduction in whole or in part is prohibited without the written
# consent of the copyright owner.

# General settings for the makefile.
#   File and folder names are case insensitive under Windows. So should behave the makefile.
.NOCASE_TARGET
# Make configuration for whitespace support.
.OPTIONS: MacroQuoted TargetQuoted

# The name of the project is used for several build products.
#   TODO Choose your project's name
project = RTuinOS

# RTuinOS can't be linked without an application. Select which one. Here, all applications
# are considered test cases.
%ifndef TEST_CASE
    %echo Please select a test case. Add TEST_CASE=\<myTestCase\> to the command line
    %abort
%endif

# The target micro controller the code is to be compiled for. The Setting is used in the
# command line of compiler, linker and flash tool. Please be aware, that changing this
# setting is not sufficient to ensure that this makefile is woring with another target. You
# definitly have to double-check all the avr tool's command lines.
targetMicroController = atmega2560

# Communication port to be used by the flash tool. The default may be adjusted here to your
# environment or you may override the variable setting on the make processor's command line.
COM_PORT ?= \\.\COM8

# Access help as default target or by several names. This target needs to be the first one
# in this file.
h help targets usage .ALWAYS:
    %echo Usage: make TEST_CASE=\<myRTuinOSApplication\> [CONFIGURATION=\<configuration\>] [COM_PORT=\<portName\>] {\<target\>}
    %echo where \<configuration\> is one out of DEBUG (default) or PRODUCTION.
    %echo and \<portName\> is an a USB port identifying string to be used for the download.
    %echo See help of avrdude for more.
    %echo Available targets are:
    %echo   build: Build the hex files for flashing onto uC. Includes all others but help
    %echo   compile: Compile all C(++) source files, but no linkage etc.
    %echo   clean: Delete all files generated by the build process
    %echo   rebuild: Same as clean and build together
    %echo   bin/\<configuration\>/obj/\<cFileName\>.obj: Compile a single C module
    %echo   download: Build first, then flash the device
    %echo   help: Print this help
    %exit 0

# Concept of compilation configurations: (TBC)
#
# Configuration PRODCUTION:
# - no self-test code
# - no debug output
# - no assertions
# 
# Configuration DEBUG:
# + all self-test code
# + debug output possible
# + all assertions active
#
CONFIGURATION ?= DEBUG
.BEFORE:
    %if "$(CONFIGURATION)" == "PRODUCTION"
      %echo Compiling production code
      %set cDefines = -D$(CONFIGURATION) -DNDEBUG
    %elseif "$(CONFIGURATION)" == "DEBUG"
      %echo Compiling debug code
      %set cDefines = -D$(CONFIGURATION)
    %else
      %error 1 Please set CONFIGURATION to either PRODUCTION or DEBUG
    %endif
    #%echo $(CONFIGURATION) $(cDefines)
    
# Where to place all generated products?
targetDir = bin\$(CONFIGURATION)

# Double-check environment.
#   The original Arduino code is referenced in the Arduino installation directory. By
# default the location of this is not known. To run this makefile an environment variable
# needs to point to the right location. This is checked now.
%if %file($(ARDUINO_HOME)\arduino.exe) != 1
    %error 1 Variable ARDUINO_HOME needs to be set prior to running this makefile. It points \
to the installation directory of the Arduino environment, where arduino.exe is located
%endif


# Ensure existence of target directory.
makeDir .ALWAYS: $(targetDir)\obj bin\core\obj
$(targetDir)\obj:
	~~mkdir $(targetDir)     2> nul
	~~mkdir $(targetDir)\obj 2> nul
bin\core\obj:
	~~mkdir $(.TARGET)     2> nul
	~~mkdir $(.TARGET)\obj 2> nul

# Determine the list of files to be compiled.
#   Specify a blank separated list of directories (better: file patterns) holding source
# files. 
srcDirList = code\RTOS\*.c?? code\sampleApp\$(TEST_CASE)\*.c??
# *F: Get all files matching the soucre file patterns in the directory list.
# F: Extract file names (incl. extension).
cFileList = $(srcDirList,*F,F)
# Exclusion list: Replace names of those files to be excluded from build by the empty
# string.
# TODO Use a list and a loop once it become more files.
cFileListExcl = $(cFileList,xxx_exampleOfExcludedFile.c=)
# Translate C source file names in target binary files by altering the extension and adding
# path information.
objList = $(cFileListExcl,.cpp=.o,.c=.o)
objListWithPath = $(objList,<$(targetDir)\obj\)
#%echo $(objListWithPath)

# Semicolon separated search paths for *.c and *.h permit to use auto rules for compilation.
.PATH.cpp = code\RTOS;code\sampleApp\$(TEST_CASE);                                  \
            $(ARDUINO_HOME)\hardware\arduino\cores\arduino;                         \
            $(ARDUINO_HOME)\libraries\LiquidCrystal
.PATH.c   = code\RTOS;code\sampleApp\$(TEST_CASE);                                  \
            $(ARDUINO_HOME)\hardware\arduino\cores\arduino
.PATH.h   = code\RTOS;code\sampleApp
# All other products is looked for in the target directory.
.PATH.d   = $(targetDir)
.PATH.eep = $(targetDir)
.PATH.hex = $(targetDir)
.PATH.elf = $(targetDir)
.PATH.lst = $(targetDir)\obj

# Inference rules for compilation of C and C++ source files.
cFlags =  $(cDefines) -c -g -O1 -Wall -fno-exceptions -ffunction-sections           \
          -fdata-sections -mmcu=$(targetMicroController) -DF_CPU=16000000L -MMD     \
          -DUSB_VID=null -DUSB_PID=null -DARDUINO=101                               \
          # TODO You may need to add more include paths here.                       \
          -Icode\RTOS -Icode\sampleApp\$(TEST_CASE)                                 \
          -I$(ARDUINO_HOME)\hardware\arduino\cores\arduino                          \
          -I$(ARDUINO_HOME)\hardware\arduino\variants\mega                          \
          -I$(ARDUINO_HOME)\libraries\LiquidCrystal                                 \
          -I$(ARDUINO_HOME)\libraries\LiquidCrystal\utility
%.o: %.c
    %echo Compiling C file $(.SOURCE)
    avr-g++ $(cFlags) -o $(.TARGET) -Wa,-a=$(.TARGET,.o=.lst) $(.SOURCE)
    
%.o: %.cpp
    %echo Compiling C++ file $(.SOURCE)
    avr-g++ $(cFlags) -o $(.TARGET) -Wa,-a=$(.TARGET,.o=.lst) $(.SOURCE)

# A general rule enforces rebuild if one of the configuration files changes
$(objListWithPath) $(objListCoreWithPath): makefile

# Compile and link all (orginal) Arduino core files into libray core.a. Although not subject
# to any changes the Arduino code is still referenced as source code for reference. Do not
# replace by a completely anonymous library.
#   The compilation of the code is implemented configuration independent - the original
# Arduino code will not know or respect our configuration dependent #defines.
objListCore = WInterrupts.o wiring.o wiring_analog.o wiring_digital.o wiring_pulse.o    \
              wiring_shift.o CDC.o HardwareSerial.o HID.o IPAddress.o new.o      		\
              Print.o Stream.o Tone.o USBCore.o WMath.o WString.o LiquidCrystal.o
objListCoreWithPath = $(objListCore,<bin\core\obj\)
#%echo $(objListCoreWithPath)
bin\core\core.a: $(objListCoreWithPath)
    avr-ar rcs $(.TARGET) $(.SOURCES)

# Let the linker create the binary ELF file.
lFlags = -Os -Wl,--gc-sections,--relax,--cref -mmcu=$(targetMicroController)
$(project).elf: compile
    %echo Linking project. Ouput is redirected to $(targetDir)\$(project).map
    avr-gcc $(lFlags) -o $(.TARGET) $(objListWithPath) bin\core\core.a -lm              \
            -Wl,-M > $(targetDir)\$(project).map

# Derive eep and hex file formats from the ELF file.
$(project).eep: $(project).elf
    avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load               \
                --no-change-warnings --change-section-lma .eeprom=0 $(.SOURCE) $(.TARGET)

$(project).hex: $(project).elf
    avr-objcopy -O ihex -R .eeprom $(.SOURCE) $(.TARGET)

# Download compiled software onto the controller.
download .ALWAYS: $(project).hex $(ARDUINO_HOME)\hardware\tools\avr\etc\avrdude.conf
    # -cWiring: The Arduino uses a quite similar protocol which unfortunately requires an
    # additional, preparatory reset command. This protocol can't therefore be applied in an
    # automated process. Here we need to use protocol Wiring instead.
    avrdude -C$(ARDUINO_HOME)\hardware\tools\avr\etc\avrdude.conf -v -v                 \
            -p$(targetMicroController) -cWiring -P$(COM_PORT) -b115200                  \
            -D -Uflash:w:$(.SOURCE):i

# Run the complete build process with compilation, linkage and a2l and binary file
# modifications.
build .ALWAYS: $(project).eep $(project).hex

# Rebuild all.
rebuild .ALWAYS: clean build

# Compile all C source files.
compile .ALWAYS: makeDir bin\core\core.a $(objListWithPath)

# Delete all products silently and without evaluation of error return code (~~).
clean .ALWAYS:
    ~~rmdir /S/Q $(targetDir) 2> nul    \
    ~~rmdir /S/Q bin\core 2> nul
