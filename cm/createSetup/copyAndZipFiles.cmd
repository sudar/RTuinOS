@echo off
:: Compose file tree, call ZIP and create the installation package.
:: Run this batch from the directory cm/createSetup only!

if "%1" == "" goto LUsage
if "%1" == "-h" goto LUsage
if "%1" == "-?" goto LUsage
if "%1" == "/?" goto LUsage
if "%1" == "/h" goto LUsage

setlocal

:: Pathes used here are relative to the root of the entire project.
cd ..\..

rmdir /S/Q tmp > nul 2> nul

:: Create the directory tree as aimed on the destination system.

mkdir tmp
mkdir tmp\RTuinOS
copy doc\readMe.forSetup.txt tmp\RTuinOS\readMe.txt

:: Global Documentation
mkdir tmp\RTuinOS\doc
mkdir tmp\RTuinOS\doc\doxygen
xcopy /S doc\doxygen\* tmp\RTuinOS\doc\doxygen
mkdir tmp\RTuinOS\doc\manual
xcopy /S doc\manual\GNUmakefile tmp\RTuinOS\doc\manual
xcopy /S doc\manual\manual.pdf tmp\RTuinOS\doc\manual
xcopy /S doc\manual\*.tex tmp\RTuinOS\doc\manual
xcopy /S doc\manual\*.jpg tmp\RTuinOS\doc\manual

:: Create a version description.
echo Archive Compilation Date:>> tmp\RTuinOS\version.txt
date /T>> tmp\RTuinOS\version.txt
time /T>> tmp\RTuinOS\version.txt

:: Protect files
:: attrib +R tmp\RTuinOS\*

:: Replace the current archive with the zipped temporary directory tree.
del /F/Q %1 > nul 2> nul
cd cm\createSetup
7za a -tzip %1 ..\..\tmp\* -r

if ERRORLEVEL 1 (
    echo Warning: Non fatal error. For example, one or more files were locked by 
    echo some other application, so they were not compressed.       
    exit /B 99
)
if ERRORLEVEL 2 (
    echo Fatal error     
    exit /B 99
)
if ERRORLEVEL 7 (
    echo Command line error     
    exit /B 99
)
if ERRORLEVEL 8 (
    echo Not enough memory for operation     
    exit /B 99
)
if ERRORLEVEL 255 (
    echo User stopped the process     
    exit /B 99
)

:: Remove the temporary directory tree again.
rmdir /S/Q ..\..\tmp

goto LEnd

:LUsage
echo usage: copyAndZipFiles archive
echo archive designates the name of the aimed archive file.
goto LEnd

:LEnd
