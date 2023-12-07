tcc -Z -O -MS -eMCC compile.c io.c pc86cg.c
del *.obj
del *.map
unpack l -h mcc.exe
