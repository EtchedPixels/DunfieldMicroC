@echo off
rem *
rem * Compile a MICRO-C library module & link into library
rem *
rem * ?COPY.TXT 1988-2005 Dave Dunfield
rem *
if .%1 == . goto help
mcc %1.C %2%1.PRE -q -f
if errorlevel 1 goto quit
mco %2%1.PRE %2%1.ASM -q
del %2%1.PRE
masm/ml/t %2%1,%2%1;
if errorlevel 1 goto quit
del %2%1.ASM
lib %2mclib +%2%1; >NUL
del %2%1.OBJ
goto quit
:help
echo Use: "%0 <filename> [tempfile path]"
:quit
