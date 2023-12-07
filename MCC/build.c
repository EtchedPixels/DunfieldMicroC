#include <stdio.h>

#define	O_VERB	0x01
#define	O_RUN	0x02

unsigned char
	*Ptr,
	*Qptr,
	Opt = O_VERB|O_RUN,
	Name[128];

unsigned char *OKname[] = {
	"PC86",
	"6809", "6811", "6812", "6816",
	"8051", "8085", "8086", "8096",
	"AVR", "CF",
	0 };

/*ChtTxt R:\Help.h
Build Micro-C Compiler (with itself)

use:	BUILD	name [options]

opts:	-Q		Quiet

"name" must be one of:
~$80
Dave Dunfield		 https://dunfield.themindfactory.com
  or: Search "Dave's Old Computers", see "personal" link.
*/
unsigned char Help[] = {
	66,117,105,108,100,32,77,105,99,114,111,45,67,32,67,111,109,112,105,108,
	101,114,32,40,119,105,116,104,32,105,116,115,101,108,102,41,10,10,
	117,115,101,58,132,66,85,73,76,68,131,110,97,109,101,32,91,111,112,
	116,105,111,110,115,93,10,10,111,112,116,115,58,131,45,81,134,81,117,
	105,101,116,10,10,34,110,97,109,101,34,32,109,117,115,116,32,98,101,
	32,111,110,101,32,111,102,58,10,128,10,68,97,118,101,32,68,117,110,
	102,105,101,108,100,136,104,116,116,112,115,58,47,47,100,117,110,102,
	105,101,108,100,46,116,104,101,109,105,110,100,102,97,99,116,111,114,
	121,46,99,111,109,10,130,111,114,58,32,83,101,97,114,99,104,32,34,
	68,97,118,101,39,115,32,79,108,100,32,67,111,109,112,117,116,101,114,
	115,34,44,32,115,101,101,32,34,112,101,114,115,111,110,97,108,34,32,
	108,105,110,107,46,10,0 };

void Pc(unsigned char c)	{	putc(c, stdout);	}
void Ps(unsigned char *p)	{	while(*p) Pc(*p++);	}
void Nl(void)				{	Pc('\n');			}

register Command(unsigned args)
{
	unsigned char buf[200];
	_format_(nargs()*2+&args, buf);
	if(Opt & O_VERB)  {
		Ps("> ");
		Ps(buf);
		Nl(); }
	if(Opt & O_RUN)
		system(buf);
}

void ShowNam(void)
{
	unsigned i;
	unsigned char *p;
	i = 0;
	while(p = OKname[i++]) {
		printf("%8s", p);
		switch(i) {
		case 1 :
		case 5 :
		case 9 :
		case 11:
			Nl(); } }
}

main(int argc, char *argv[])
{
	unsigned i;
	unsigned char c;

	i = 0;
	while(++i < argc) {
		if(*(Ptr = argv[i]) == '-') {
			++Ptr;
o1:			switch(toupper(*Ptr++)) {
			default	:	goto he;
			case 'Q':	c = O_VERB;
				Opt ^= c; }
			if(*Ptr) goto o1;
			continue; }
		if(*Name) goto he;
		strcpy(Name, Ptr);
		strupr(Name); }

	if(!*Name) {
he:		Ptr = Help;
		while(c = *Ptr++) {
			if(c & 0x80) {
				if(c == 0x80) {
					ShowNam();
					continue; }
				while(c-- & 0x7F)
					Pc(' ');
				continue; }
			Pc(c); }
		return; }

	i = 0;
	while(Ptr = OKname[i++]) {
		if(!strcmp(Ptr, Name))
			goto a1; }
	printf("Name '%s' not recognized!\n");
	goto he;
a1:
	Qptr = (Opt & O_VERB) ? "" : "Q";
	Command("cc compile -pofm%s", Qptr);
	Command("cc io -pofm%s CPU=%s", Qptr, Name);
	Command("cc CODEGEN\\%scg -pofm%s", Name, Qptr);
	Command("lc -s compile io %scg", Name);
	Command("del *.OBJ");
	if(Opt & O_VERB)
		Ps("Don't forget to rename COMPILE.EXE to MCC?cpu?.EXE\n");
}
