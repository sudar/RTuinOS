# TODO:
#   Test "pit-fall" with too large code size for short jumps

#
# Generic Makefile for Arduino Project
#
# Compilation and linkage of C(++) code into binary files and upload to the controller.
#
# Help on the syntax of this makefile is got at
# http://www.gnu.org/software/make/manual/make.pdf.
#
# Copyright (C) 2012-2013 Peter Vranken (mailto:Peter_Vranken@Yahoo.de)
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
#   The name of the project can be assigned to the makefile macro project, see heading part
# of the code section of this makefile. If you don't do, your compilation products will use
# a standard name.
#   The makefile hard codes the hardware target. Among more, see makefile macros
# targetMicroController, cFlags and lFlags. Here, you will have to make some changes
# according to your selection of an ATmega micro controller. More changes will be required
# to the command lines of the object file tool avr-objcopy and the flash tool avrdude.
#   Hint: To find out how to run these tools, you can enable verbose mode in the Arduino
# IDE and compile one of the code examples. Build and upload the sketch in the IDE, then
# copy the contents of the IDE's output window and paste them into a text editor. You will
# find appropriate command lines for all the tools.
#   The Arduino installation directory needs to be referenced. The location is determined
# by environment variable ARDUINO_HOME. The variable holds the name of the folder which
# arduino.exe is located in. Caution: This variable is not created by the original Arduino
# installation process but needs to be created manually.
#   For your convenience, the Windows path should contain the location of the GNU make
# processor. If you name this file either makefile or GNUmakefile you will just have to
# type "make" in order to get your make process running. Typically, the path to the
# executable is $(ARDUINO_HOME)hardware/tools/avr/utils/bin. Consider to extend the
# Windows environment variable PATH accordingly.
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
#   CONFIG: The compile configuration is one out of DEBUG (default) or PRODUCTION. By
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
COM_PORT ?= COM8

# Exclusion list: Basically all C/C++ source files found in the source directories are
# compiled and linked. Here, you can specify some particular files, which must not be
# included in the build.
#   TODO Edit the blank separated list of excluded file names (without path).
cFileListExcl := dummy.cpp


# RTuinOS can't be linked without an application. Select which one. Here, all applications
# are considered test cases.
APP ?= TC01
ifneq ($(origin APP), command line)
    $(warning Please select an RTuinOS application. Add APP=<myRTuinOSApp> to the command line, otherwise APP=$(APP) will be used)
endif

# Access help as default target or by several names. This target needs to be the first one
# in this file.
.PHONY: h help targets usage
h help targets usage:
	$(info Usage: make [-s] APP=<myRTuinOSApplication> [CONFIG=<configuration>] [COM_PORT=<portName>] {<target>})
	$(info where <myRTuinOSApplication> is the name of the source code folder of your application, located at code/applications)
	$(info and where <configuration> is one out of DEBUG (default) or PRODUCTION)
	$(info and <portName> is an a USB port identifying string to be used for the upload.)
	$(info The default port ($(COM_PORT)) is configured in the makefile. See help of avrdude for more.)
	$(info Available targets are:)
	$(info   - build: Build the hex files for flashing onto uC)
	$(info   - clean: Delete all application files generated by the build process)
	$(info   - cleanCore: Delete the compilation core.a of the Arduino standard library files)
	$(info   - rebuild: Same as clean and build together)
	$(info   - bin/<configuration>/obj/<cFileName>.o: Compile a single C(++) module)
	$(info   - upload: Build first, then flash the device)
	$(info   - makeDir: Create folder structure for generated files)
	$(info   - help: Print this help)
	$(error)

# Concept of compilation configurations:
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
CONFIG ?= DEBUG
ifeq ($(CONFIG), PRODUCTION)
    $(info Compiling production code)
    cDefines := -D$(CONFIG) -DNDEBUG
else ifeq ($(CONFIG), DEBUG)
    $(info Compiling debug code)
    cDefines := -D$(CONFIG)
else
    $(error Please set CONFIG to either PRODUCTION or DEBUG)
endif
#$(info $(CONFIG) $(cDefines))

# Where to place all generated products?
targetDir := bin/$(APP)/$(CONFIG)/
coreDir := bin/core/

# Double-check environment.
#   The original Arduino code is referenced in the Arduino installation directory. By
# default the location of this is not known. To run this makefile an environment variable
# needs to point to the right location. This is checked now.
ifdef ARDUINO_HOME
    # Ensure a trailing slash at the end of this externally set variabale.
	ARDUINO_HOME := $(patsubst %/,%,$(subst \,/,$(ARDUINO_HOME)))/
else
    $(error Variable ARDUINO_HOME needs to be set prior to running this makefile. It points \
to the installation directory of the Arduino environment, where arduino.exe is located)
endif


# Ensure existence of target directory.
.PHONY: makeDir
makeDir: | $(targetDir)obj $(coreDir)obj

$(targetDir)obj $(coreDir)obj:
	-$(mkdir) -p $@

# Determine the list of files to be compiled.
#   Specify a blank separated list of directories holding source files.
srcDirList := code/RTOS/ code/applications/$(APP)/
# Create a blank separated list file patterns matching possible source files.
srcPatternList := $(foreach path, $(srcDirList), $(addprefix $(path), *.c *.cpp))
# Get all files matching the source file patterns in the directory list. Caution: The
# wildcard function will not accept Windows style paths.
cFileList := $(wildcard $(srcPatternList))
# Remove the various paths. We assume unique file names across paths and will search for
# the files later. This strongly simplyfies the compilation rules. (If source file names
# were not unique we could by the way not use a shared folder obj for all binaries.)
cFileList := $(notdir $(cFileList))
# Exclusion list: Replace names of those files to be excluded from build by the empty
# string. Subtract each excluded file from the list.
cFileList := $(filter-out $(cFileListExcl), $(cFileList))
#$(info cFileList := $(cFileList))
# Translate C source file names in target binary files by altering the extension and adding
# path information.
objList := $(cFileList:.cpp=.o)
objList := $(objList:.c=.o)
objListWithPath := $(addprefix $(targetDir)obj/, $(objList))
#$(info objListWithPath := $(objListWithPath))

# Include the dependency files. Do this with a failure tolerant include operation - the
# files are not available after a clean.
-include $(patsubst %.o,%.d,$(objListWithPath))

# Blank separated search path for source files and their prerequisites permits to use auto
# rules for compilation.
VPATH := $(srcDirList) 																\
         $(targetDir)                                                               \
         $(ARDUINO_HOME)hardware/arduino/cores/arduino/                             \
         $(ARDUINO_HOME)libraries/LiquidCrystal/

# Pattern rules for compilation of C and C++ source files.
#   TODO You may need to add more include paths here.
cFlags =  $(cDefines) -c -Wall -fno-exceptions -ffunction-sections                  \
          -fdata-sections -mmcu=$(targetMicroController) -DF_CPU=16000000L -MMD     \
          -DUSB_VID=null -DUSB_PID=null -DARDUINO=101                               \
          -Wa,-a=$(patsubst %.o,%.lst,$@)                                           \
          -Winline                                                                  \
          $(foreach path, $(srcDirList), -I$(path))                                 \
          -I$(ARDUINO_HOME)hardware/arduino/cores/arduino/                          \
          -I$(ARDUINO_HOME)hardware/arduino/variants/mega/                          \
          -I$(ARDUINO_HOME)libraries/LiquidCrystal/                                 \
          -I$(ARDUINO_HOME)libraries/LiquidCrystal/utility/
ifeq ($(CONFIG),DEBUG)
	cDbgFlags := -ggdb3 -Os
else
	cDbgFlags := -g -Os
endif
#$(info cFlags := $(cFlags))

$(targetDir)obj/%.o: %.c
	$(info Compiling C file $<)
	$(avr-g++) $(cDbgFlags) $(cFlags) -o $@ $<

$(targetDir)obj/%.o: %.cpp
	$(info Compiling C++ file $<)
	$(avr-g++) $(cDbgFlags) $(cFlags) -o $@ $<


# Compile and link all (original) Arduino core files into libray core.a. Although not
# subject to any changes the Arduino code is still referenced as source code for reference.
# Do not replace by a completely anonymous library.
#   The compilation of the code is implemented configuration independent - the original
# Arduino code will not know or respect our configuration dependent #defines.
objListCore = WInterrupts.o wiring.o wiring_analog.o wiring_digital.o wiring_pulse.o    \
              wiring_shift.o CDC.o HardwareSerial.o HID.o IPAddress.o new.o      		\
              Print.o Stream.o Tone.o USBCore.o WMath.o WString.o LiquidCrystal.o
objListCoreWithPath = $(addprefix $(coreDir)obj/, $(objListCore))
#$(info objListCoreWithPath := $(objListCoreWithPath))

$(coreDir)obj/%.o: %.c
	$(info Compiling C file $<)
	$(avr-g++) -g -Os $(cFlags) -o $@ $<

$(coreDir)obj/%.o: %.cpp
	$(info Compiling C++ file $<)
	$(avr-g++) -g -Os $(cFlags) -o $@ $<

$(coreDir)core.a: $(objListCoreWithPath)
	$(info Creating Arduino standard library $@)
	$(avr-ar) rcs $@ $^


# A general rule enforces rebuild if one of the configuration files changes
#$(objListWithPath) $(objListCoreWithPath): GNUmakefile

# Let the linker create the binary ELF file.
#   Here we have a serious pit-fall. We know from the Arduino IDE, that the compiled
# Arduino standard files are bundled in the archive file core.a and that the program is
# linked against this archive. We started doing the same. Following problem appears: The
# vector table at the beginning of the code section uses short 12 Bit jumps to the
# implementation of the interrupt service routines. These jumps can't be changed, as
# there's no source code available. If we place core.a behind the object files of the
# program the size of the program is limited by the 12 Bit jump from before till behind -
# which is unacceptable. If we place core.a in front of the program's object files we end
# up with numerous unresolved references since the linker doesn't seem to do two passes
# across an archive as naturally for a set of object files. The work around is to not use
# the archive but to place all the distinct object files into the command line. Now the
# object files from the Arduino files may come first (i.e. safely inside the 12 Bit range)
# followed by the object files of the program itself. We keep the file core.a nonetheless
# but only as a kind of structural element of the compilation process. It is used as target
# of the Arduino file compilation and as prerequisite for the linkage of the RTuinOS
# application. If it is up-to-date, we can be sure, that all Arduino object file are also
# up-to-date.
#   Remark: It seems that only the vectors to the ISRs in HardwareSerial use short jumps.
# The implementation of some other ISRs (e.g. in rtos.o) is definitly allowed to be more
# than 4 kByte away from the vector table.
#   TODO The loader permits the usage of "--start-group archives --end-group" and it
# permits to place the same archive repeatedly in the command line. These options should
# solve the problem.
#   Done: Both possibilities are accepted by the linker, the SW is built. However,
# currently we don't have any example, where we would run into the mentioned problem - all
# sample's code is too little. Therefore, we removed the first use of $< (before
# $(objListWithPath)) in order to provoke the error in the future. If it pops up, we will
# see if it disappears by reinserting the $<. The previous work around as commented above
# looked like:
#	$(avr-gcc) $(lFlags) -o $@ $(objListCoreWithPath) $(objListWithPath) -lm       	\
#              -Wl,-M > $(targetDir)$(project).map
lFlags = -Os -Wl,--gc-sections,--relax,--cref -mmcu=$(targetMicroController)		\
         --fatal-warnings --no-undefined --reduce-memory-overheads --stats
$(targetDir)$(project).elf: $(coreDir)core.a $(objListWithPath)
	$(info Linking project. Ouput is redirected to $(targetDir)$(project).map)
	$(avr-gcc) $(lFlags) -o $@ $(objListWithPath) $(coreDir)core.a -lm       		\
               -Wl,-M > $(targetDir)$(project).map
	$(avr-size) -C --mcu=$(targetMicroController) $@

# Derive eep and hex file formats from the ELF file.
$(targetDir)$(project).eep: $(targetDir)$(project).elf
	$(avr-objcopy) -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load        \
                --no-change-warnings --change-section-lma .eeprom=0 $< $@

$(targetDir)$(project).hex: $(targetDir)$(project).elf
	$(avr-objcopy) -O ihex -R .eeprom $< $@

# Upload compiled software on the controller.
#   Option -cWiring: The Arduino IDE uses a quite similar protocol which unfortunately
# requires an additional, preparatory reset command. This protocol can't therefore be
# applied in an automated process. Here we need to use protocol Wiring instead.
.PHONY: upload
upload: makeDir																				\
        $(targetDir)$(project).hex $(targetDir)$(project).elf $(targetDir)$(project).eep	\
        $(ARDUINO_HOME)hardware/tools/avr/etc/avrdude.conf
	$(avrdude) -C$(ARDUINO_HOME)hardware/tools/avr/etc/avrdude.conf -v                      \
	        -p$(targetMicroController) -cWiring -P$(COM_PORT) -b115200 -D                   \
            -Uflash:w:$(targetDir)$(project).hex:i
	$(avr-size) -C --mcu=$(targetMicroController) $(targetDir)$(project).elf


# Run the complete build process with compilation, linkage and binary file modifications.
.PHONY: build
build: makeDir $(targetDir)$(project).eep $(targetDir)$(project).hex

# Rebuild all.
.PHONY: rebuild
rebuild: clean build

# Compile all C source files.
.PHONY: compile
compile: makeDir $(coreDir)core.a $(objListWithPath)

# Delete all application products ignoring (-) the return code from Windows.
.SILENT: clean
.PHONY: clean
clean:
	-$(rm) -f $(targetDir)$(project).* 2> nul
	-$(rm) -fr $(targetDir)obj 2> nul

# Delete the core compilation (Arduino standard files) ignoring (-) the return code from
# Windows. This target is not part of clean as rebuilding the Arduino library is usually
# not necessary during development of an application - you won't alter any Arduino files.
# An exception would of course be the cahnge of a compilation flag, but changing the
# makefile will anyway enforce a rebuild also of core.a.
.SILENT: cleanCore
.PHONY: cleanCore
cleanCore:
	-$(rm) -r $(coreDir)core.a 2> nul
	-$(rm) -fr $(coreDir)obj 2> nul

# Quite typical, the GNU tools reside on a system at several locations as they come along
# with many applications. Hard to locate problems due to arbitrary order of references in
# the Windows search PATH can easily reuslt. To avoid these problems we reference all tools
# by absolute path. The path is known as we have the Arduino installation directory.
make := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/make.exe
mkdir := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/mkdir.exe
rmdir := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/rmdir.exe
cat := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/cat.exe
echo := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/echo.exe
rm := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/rm.exe
gawk := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/gawk.exe
awk := $(gawk)
touch := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/touch.exe
mv := $(ARDUINO_HOME)hardware/tools/avr/utils/bin/mv.exe
avr-gcc := $(ARDUINO_HOME)hardware/tools/avr/bin/avr-gcc.exe
avr-g++ := $(ARDUINO_HOME)hardware/tools/avr/bin/avr-g++.exe
avr-ar := $(ARDUINO_HOME)hardware/tools/avr/bin/avr-ar.exe
avr-objcopy := $(ARDUINO_HOME)hardware/tools/avr/bin/avr-objcopy.exe
avr-size := $(ARDUINO_HOME)hardware/tools/avr/bin/avr-size.exe
avrdude := $(ARDUINO_HOME)hardware/tools/avr/bin/avrdude
