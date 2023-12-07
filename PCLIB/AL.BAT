@echo off
rem *
rem * Assemble a MICRO-C library module & link into library
rem *
rem * ?COPY.TXT 1988-2005 Dave Dunfield
rem *
if .%1 == . goto help
masm/ml/t %1,%2%1;
if errorlevel 1 goto quit
lib %2mclib +%2%1; >NUL
del %2%1.OBJ
goto quit
:help
echo Use: "%0 <filename> [tempfile path]"
:quit
