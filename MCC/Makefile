all: dcc6808 dcc6809 dcc6811 dcc6812 dcc6816 dcc8051 dcc8085 dcc8086 dcc8096 dccavr dccpc86 dcccf

OBJ = io.o compile.o

CFLAGS += -I.. -I.

dcc6808: $(OBJ) CODEGEN/6808cg.o
	cc $^ -o $@

dcc6809: $(OBJ) CODEGEN/6809cg.o
	cc $^ -o $@

dcc6811: $(OBJ) CODEGEN/6811cg.o
	cc $^ -o $@

dcc6812: $(OBJ) CODEGEN/6812cg.o
	cc $^ -o $@

dcc6816: $(OBJ) CODEGEN/6816cg.o
	cc $^ -o $@

dcc8051: $(OBJ) CODEGEN/8051cg.o
	cc $^ -o $@

dcc8085: $(OBJ) CODEGEN/8085cg.o
	cc $^ -o $@

dcc8086: $(OBJ) CODEGEN/8086cg.o
	cc $^ -o $@

dcc8096: $(OBJ) CODEGEN/8096cg.o
	cc $^ -o $@

dccavr: $(OBJ) CODEGEN/avrcg.o
	cc $^ -o $@

dccpc86: $(OBJ) CODEGEN/pc86cg.o
	cc $^ -o $@

dcccf: $(OBJ) CODEGEN/cfcg.o
	cc $^ -o $@

