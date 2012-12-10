@echo off
:: See usage message for help. See
:: http://en.wikibooks.org/wiki/Windows_Programming/Programming_CMD for syntax of batch
:: scripts.
goto LStart
:LUsage
echo usage: makeDoc
echo   Generate the (dependent) documentation files, which are part of the setup but which 
echo are not under source control. This script is part of the setup generation process. The
echo start directory is cm\createSetup.
goto :eof

:: 
:: Copyright (c) 2012, FEV GmbH, Germany
:: 
:: Author: Peter Vranken (mailto:Peter.Vranken@FEV.de)
:: 

:LStart
::if /i "%1" == "" goto LUsage
if /i "%1" == "-h" goto LUsage
if /i "%1" == "-?" goto LUsage
if /i "%1" == "/?" goto LUsage
if /i "%1" == "/h" goto LUsage
:: Limit the allowed number of parameters.
if not "%1" == "" goto LUsage

setlocal

pushd "..\..\doc\manual"
make
popd
pushd "..\..\doc\doxygen"
doxygen
popd




