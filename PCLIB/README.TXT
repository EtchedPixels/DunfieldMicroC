DDS MICRO-C/PC library source code:

This directory contains the source code to the MICRO-C library for IBM-PC
compatibles.

The functions in this directory are being provided for your convenience in
understanding the MICRO-C/PC library and modifying it to suit your individual
requirements. The functions are for personal use only, any commercial use of
this software is prohibited.

The functions are provided "as is", without warranty of any kind. Any use
you make of them is entirely at your own risk. Neither Dunfield Development
Services nor any of the authors of these programs assume any responsibility
for the use or reliability of this software.

NOTE: MANY FUNCTIONS IN THIS LIBRARY RELY ON THE CALLING CONVENTIONS OR
OTHER UNIQUE FEATURES OF THE MICRO-C COMPILER, AND WILL REQUIRE EXTENSIVE
MODIFICATION TO WORK WITH ANY OTHER COMPILER.

I have provided two "BATCH" command files: CL.BAT and AL.BAT, which will
compile (or assemble) an individual .C (or .ASM) library file, and add it
to a MCLIB.LIB library file located on the specified temporary path.
NOTE: If you are replacing a library function, you must REMOVE it from the
library BEFORE you run CL.BAT or AL.BAT!

To build the library "from scratch", you must run CL.BAT on every .C file,
and AL.BAT on every .ASM file. The resultant MCLIB.LIB file should be
placed in the MICRO-C home directory.

I have also included the source code to the runtime library startup code
in the file PC86.RL; A batch file called "MAKERL.BAT: is provided which
will build two object files: PC86RL_T.OBJ and PC86RL_S.OBJ which are required
for linking the TINY and SMALL memory models. These object files should be
placed in the MICRO-C home directory.

Dave Dunfield
Dunfield Development Services
