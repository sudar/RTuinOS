@echo off
:: Compose file tree, call ZIP and create the installation package.
:: Run this batch from the directory cm/createSetup only!

if "%1" == "" goto LUsage
if "%1" == "-h" goto LUsage
if "%1" == "-?" goto LUsage
if "%1" == "/?" goto LUsage
if "%1" == "/h" goto LUsage

setlocal

rmdir /S/Q tmp > nul 2> nul

:: Create the directory tree as aimed on the destination system.

mkdir tmp
mkdir tmp\RTuinOS

:: Pathes used for collecting the file are relative to the root of the project.
pushd ..\..
set output=cm\createSetup\tmp\RTuinOS

:: Global Documentation
copy doc\readMe.forSetup.txt %output%\readMe.txt
mkdir %output%\doc
mkdir %output%\doc\doxygen
xcopy /S doc\doxygen\* %output%\doc\doxygen
mkdir %output%\doc\manual
xcopy /S doc\manual\GNUmakefile %output%\doc\manual
xcopy /S doc\manual\manual.pdf %output%\doc\manual
xcopy /S doc\manual\*.tex %output%\doc\manual
xcopy /S doc\manual\*.jpg %output%\doc\manual

:: Create a version description.
echo Archive Compilation Date:>> %output%\version.txt
date /T>> %output%\version.txt
time /T>> %output%\version.txt

:: Protect files
attrib +R %output%\*

:: Replace the current archive with the zipped temporary directory tree.
popd
del /F/Q %1 > nul 2> nul
7za a -tzip %1 .\tmp\* -r
if ERRORLEVEL 1 (
    echo Error: Files could not be archived completely
    exit /B 1
)

:: Remove the temporary directory tree again.
rmdir /S/Q tmp

goto LEnd

:LUsage
echo usage: copyAndZipFiles archive
echo archive designates the name of the aimed archive file.
goto LEnd

:LEnd
