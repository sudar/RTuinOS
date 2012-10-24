# 
# Generic Makefile for Arduino Project
#
# Compilation and linkage of C(++) code into binary files and download to the controller.
#
# Help on the syntax of this makefile is got at
# http://www.gnu.org/software/make/manual/make.pdf.
#
# Copyright (C) 2012 Peter Vranken (mailto:Peter_Vranken@Yahoo.de)
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by the
# Free Software Foundation, either version 3 of the License, or any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Preconditions
# =============
# 
# The makefile is intended to be executed by the GNU make utility coming along with the
# Arduino package.
#   The name of the project needs to be assigned to the makefile macro projectName, see
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
#   For your convenience, the Windows path should contain the location of the GNU make
# processor. If you name this file either makefile or GNUmakefile you will just have to
# type "make" in order to get your make process running. Typically, the path to the
# executable is $(ARDUINO_HOME)\hardware\tools\avr\utils\bin
#   This makefile does not handle blanks in any paths or file names. Please rename your
# paths and files accordingly prior to using this makefile.
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
#   A second list of files is found as cFileListExcl. These C/C++ files are excluded from
# build.
#   Additionally required Arduino library files are hard coded in this makefile. They are
# referenced by absolute paths into the Arduino installation directory. Therefore, the
# installation directory needs to be known by means of an environment variable called
# ARDUINO_HOME.
#   These settings are invariant throughout the project life time and don't need
# maintenance.


# General settings for the makefile.
#$(info Makeprocessor in use is $(MAKE))
SHELL = cmd
.SHELLFLAGS = /c

# The name of the project is used for several build products.
project = RTuinOS_$(APP)

# The target micro controller the code is to be compiled for. The Setting is used in the
# command line of compiler, linker and flash tool. Please be aware, that changing this
# setting is not sufficient to ensure that this makefile is woring with another target. You
# definitly have to double-check all the avr tool's command lines.
targetMicroController := atmega2560

# Communication port to be used by the flash tool. The default may be adjusted here to your
# environment or you may override the variable setting on the make processor's command line.
COM_PORT ?= \\.\COM8

# RTuinOS can't be linked without an application. Select which one. Here, all applications
# are considered test cases.
APP ?= TC01
ifneq ($(origin APP), command line)
    $(warning Please select an RTuinOS application. Add APP=<myRTuinOSApp> to the command line, otherwise $(APP) will be used)
endif

# Access help as default target or by several names. This target needs to be the first one
# in this file.
.PHONY: h help targets usage
h help targets usage:
	$(info Usage: make APP=<myRTuinOSApplication> [CONFIGURATION=<configuration>] [COM_PORT=<portName>] {<target>})
	$(info where <myRTuinOSApplication> is the name of the source code folder of your application, located at code\applications)
	$(info and where <configuration> is one out of DEBUG (default) or PRODUCTION)
	$(info and <portName> is an a USB port identifying string to be used for the download.)
	$(info The default port is configured in the makefile. See help of avrdude for more.)
	$(info Available targets are:)
	$(info   - build: Build the hex files for flashing onto uC. Includes all others but help)
	$(info   - compile: Compile all C(++) source files, but no linkage etc.)
	$(info   - clean: Delete all application files generated by the build process)
	$(info   - cleanCore: Delete the compilation core.a of the Arduino standard library files)
	$(info   - makeDir: Create folder structure for generated files. Needs to be called first)
	$(info   - rebuild: Same as clean and build together)
	$(info   - bin/<configuration>/obj/<cFileName>.o: Compile a single C(++) module)
	$(info   - download: Build first, then flash the device)
	$(info   - help: Print this help)
	$(error)

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
ifeq ($(CONFIGURATION), PRODUCTION)
    $(info Compiling production code)
    cDefines := -D$(CONFIGURATION) -DNDEBUG
else ifeq ($(CONFIGURATION), DEBUG)
    $(info Compiling debug code)
    cDefines := -D$(CONFIGURATION)
else
    $(error Please set CONFIGURATION to either PRODUCTION or DEBUG)
endif
#$(info $(CONFIGURATION) $(cDefines))

# Where to place all generated products?
targetDir := bin\$(CONFIGURATION)
coreDir := bin\core

# Double-check environment.
#   The original Arduino code is referenced in the Arduino installation directory. By
# default the location of this is not known. To run this makefile an environment variable
# needs to point to the right location. This is checked now.
ifndef ARDUINO_HOME
    $(error Variable ARDUINO_HOME needs to be set prior to running this makefile. It points \
to the installation directory of the Arduino environment, where arduino.exe is located)
endif


# Ensure existence of target directory.
.PHONY: makeDir
makeDir: | $(targetDir)\obj $(coreDir)\obj
$(targetDir)\obj:
	-mkdir $(targetDir)     2> nul
	-mkdir $(targetDir)\obj 2> nul
$(coreDir)\obj:
	-mkdir $(coreDir)     2> nul
	-mkdir $(coreDir)\obj 2> nul

# Determine the list of files to be compiled.
#   Specify a blank separated list of directories holding source files.
srcDirList := code\RTOS code\applications\$(APP) #
# Create a blank separated list file patterns matching possible source files.
srcPatternList := $(foreach path, $(srcDirList), $(addprefix $(path), \*.c \*.cpp))
# Get all files matching the source file patterns in the directory list. Caution: The
# wildcard function will not accept Windows style paths.
cFileList := $(wildcard $(subst \,/, $(srcPatternList)))
# Remove the various paths. We assume unique file names across paths and will search for
# the files later. This strongly simplyfies the compilation rules. (If source file names
# were not unique we could by the way not use a shared folder obj for all binaries.)
cFileList := $(notdir $(cFileList))
# Exclusion list: Replace names of those files to be excluded from build by the empty
# string.
#   TODO Edit the blank separated list of excluded files.
cFileListExcl := dummy.cpp
# Subtract each excluded file from the list.
cFileList := $(filter-out $(cFileListExcl), $(cFileList))
#$(info cFileList := $(cFileList))
# Translate C source file names in target binary files by altering the extension and adding
# path information.
objList := $(cFileList:.cpp=.o)
objList := $(objList:.c=.o)
objListWithPath := $(addprefix $(targetDir)\obj\, $(objList))
#$(info objListWithPath := $(objListWithPath))

# Blank separated search path for source files and their prerequisites permit to use auto
# rules for compilation.
VPATH := $(srcDirList) 																\
         $(targetDir)                                                               \
         $(ARDUINO_HOME)\hardware\arduino\cores\arduino                             \
         $(ARDUINO_HOME)\libraries\LiquidCrystal

# Pattern rules for compilation of C and C++ source files.
#   TODO You may need to add more include paths here.
cFlags =  $(cDefines) -c -g -Os -Wall -fno-exceptions -ffunction-sections           \
          -fdata-sections -mmcu=$(targetMicroController) -DF_CPU=16000000L -MMD     \
          -DUSB_VID=null -DUSB_PID=null -DARDUINO=101                               \
          -Wa,-a=$(patsubst %.o,%.lst,$@)                                           \
          $(foreach path, $(srcDirList), -I$(path))                                 \
          -I$(ARDUINO_HOME)\hardware\arduino\cores\arduino                          \
          -I$(ARDUINO_HOME)\hardware\arduino\variants\mega                          \
          -I$(ARDUINO_HOME)\libraries\LiquidCrystal                                 \
          -I$(ARDUINO_HOME)\libraries\LiquidCrystal\utility

$(targetDir)\obj\\%.o: %.c
	$(info Compiling C file $<)
	avr-g++ $(cFlags) -o $@ $<

$(targetDir)\obj\\%.o: %.cpp
	$(info Compiling C++ file $<)
	avr-g++ $(cFlags) -o $@ $<


# Compile and link all (original) Arduino core files into libray core.a. Although not
# subject to any changes the Arduino code is still referenced as source code for reference.
# Do not replace by a completely anonymous library.
#   The compilation of the code is implemented configuration independent - the original
# Arduino code will not know or respect our configuration dependent #defines.
objListCore = WInterrupts.o wiring.o wiring_analog.o wiring_digital.o wiring_pulse.o    \
              wiring_shift.o CDC.o HardwareSerial.o HID.o IPAddress.o new.o      		\
              Print.o Stream.o Tone.o USBCore.o WMath.o WString.o LiquidCrystal.o
objListCoreWithPath = $(addprefix $(coreDir)\obj\, $(objListCore))
#$(info objListCoreWithPath := $(objListCoreWithPath))

$(coreDir)\obj\\%.o: %.c
	$(info Compiling C file $<)
	avr-g++ $(cFlags) -o $@ $<

$(coreDir)\obj\\%.o: %.cpp
	$(info Compiling C++ file $<)
	avr-g++ $(cFlags) -o $@ $<

$(coreDir)\core.a: $(objListCoreWithPath)
	avr-ar rcs $@ $^


# A general rule enforces rebuild if one of the configuration files changes
$(objListWithPath) $(objListCoreWithPath): GNUmakefile

# Let the linker create the binary ELF file.
lFlags = -Os -Wl,--gc-sections,--relax,--cref -mmcu=$(targetMicroController)
$(targetDir)\$(project).elf: $(coreDir)\core.a $(objListWithPath)
	$(info Linking project. Ouput is redirected to $(targetDir)\$(project).map)
	avr-gcc $(lFlags) -o $@ $(objListWithPath) $(coreDir)\core.a -lm                    \
            -Wl,-M > $(targetDir)\$(project).map

# Derive eep and hex file formats from the ELF file.
$(targetDir)\$(project).eep: $(targetDir)\$(project).elf
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load               \
                --no-change-warnings --change-section-lma .eeprom=0 $< $@

$(targetDir)\$(project).hex: $(targetDir)\$(project).elf
	avr-objcopy -O ihex -R .eeprom $< $@

# Download compiled software onto the controller.
#   Option -cWiring: The Arduino uses a quite similar protocol which unfortunately requires
# an additional, preparatory reset command. This protocol can't therefore be applied in an
# automated process. Here we need to use protocol Wiring instead.
.PHONY: download
download: $(targetDir)\$(project).hex $(targetDir)\$(project).eep                       \
          $(ARDUINO_HOME)\hardware\tools\avr\etc\avrdude.conf
	avrdude -C$(ARDUINO_HOME)\hardware\tools\avr\etc\avrdude.conf -v                    \
	        -p$(targetMicroController) -cWiring -P$(COM_PORT) -b115200 -D -Uflash:w:$<:i

# Run the complete build process with compilation, linkage and a2l and binary file
# modifications.
.PHONY: build
build: $(targetDir)\$(project).eep $(targetDir)\$(project).hex

# Rebuild all.
.PHONY: rebuild
rebuild: clean build

# Compile all C source files.
.PHONY: compile
compile: $(coreDir)\core.a $(objListWithPath)

# Delete all application products ignoring (-) the return code from Windows.
#   Caution: Using rm requires folder arduino-1.0.1\hardware\tools\avr\utils\bin to be in
# the Windows search path.
#	rm.exe -fr $(targetDir)
.SILENT: clean
.PHONY: clean
clean:
	-del /S/Q $(targetDir)\*.* 2> nul > nul

# Delete the core compilation (Arduino standard files) ignoring (-) the return code from
# Windows. This target is not part of clean as rebuilding the Arduino library is usually
# not necessary during development of an application - you won't alter any Arduino files.
# An exception would of course be the cahnge of a compilation flag, but changing the
# makefile will anyway enforce a rebuild also of core.a.
#   Caution: Using rm requires folder arduino-1.0.1\hardware\tools\avr\utils\bin to be in
# the Windows search path.
#	rm.exe -fr $(coreDir)
.SILENT: cleanCore
.PHONY: cleanCore
cleanCore:
	-del /S/Q $(coreDir)\*.*   2> nul > nul
