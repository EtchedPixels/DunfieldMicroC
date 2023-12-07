/*
 * Non-Recursive ANSI Quicksort function
 *
 * Public domain by Raymond Gardner, Englewood CO  February 1991
 * Adapted for MICRO-C compatibility by Dave Dunfield
 *
 * Subfiles of T or fewer elements will be sorted by a simple
 * insertion sort.  Note: T must be at least 3!
 */
#define  T		7

/* Internal function to swap one element with another */
static unsigned swapsize;	/* Record size for element exchanges */
static swap(char *ptr1, char *ptr2)
{
	unsigned size;
	char t;
	size = swapsize;
	do {
		t = *ptr1;
		*ptr1++ = *ptr2;
		*ptr2++ = t; }
	while(--size);
}

/* main quicksort function */
qsort(base, elements, size, comp)
	unsigned char *base;	/* Base address of structure to sort */
	unsigned elements;		/* Number of elements in structure */
	unsigned size;			/* Size of each element */
	int *comp;				/* Pointer to compare function */
{
	char *stack[40], **sp;			/* stack and stack pointer		*/
	char *i, *j, *limit;			/* scan and limit pointers		*/
	unsigned thresh;				/* size of T elements in bytes	*/

	thresh = (swapsize=size)*T;		/* init threshold				*/
	sp = stack;						/* init stack pointer			*/
	limit = base+elements*size;		/* pointer past end of array	*/
	for(;;) {						/* repeat until break...		*/
		if(limit - base > thresh) {	/* if more than T elements  */
			/*	swap base with middle  */
			swap((((limit-base)/size)/2)*size+base, base);
			i = base + size;			/* i scans left to right	*/
			j = limit - size;			/* j scans right to left	*/
			if((*comp)(i, j) > 0)		/* Sedgewick's				*/
				swap(i, j);				/*	three-element sort		*/
			if((*comp)(base, j) > 0)	/*	sets things up	 		*/
				swap(base, j);			/*	so that					*/
			if((*comp)(i, base) > 0)	/*		*i <= *base <= *j	*/
				swap(i, base);			/* *base is pivot element	*/
			for(;;) {					/* loop until break			*/
				do						/* move i right				*/
					i += size;			/*		  until *i >= pivot */
				while((*comp)(i, base) < 0);
				do						/* move j left				*/
					j -= size;			/*	until *j <= pivot 		*/
				while((*comp)(j, base) > 0);
				if(i > j)				/* if pointers crossed		*/
					break;				/*	break loop				*/
				swap(i, j); }	 /* else swap elements,keep scanning*/
			swap(base, j);			/* move pivot into correct place*/
			if(j - base > limit - i) {	/* if left subfile larger	*/
				sp[0] = base;			/* stack left subfile base  */
				sp[1] = j;				/*	and limit				*/
				base = i; }				/* sort the right subfile	*/
			else {						/* else right subfile larger*/
				sp[0] = i;				/* stack right subfile base */
				sp[1] = limit;			/*	 and limit				*/
				limit = j; }			/* sort the left subfile	*/
			sp += 2*sizeof(char*); }	/* increment stack pointer  */
		else {			/* else subfile is small, use insertion sort*/
			for(j = base, i = j+size; i < limit; j = i, i += size)
				while((*comp)(j, j+size) > 0) {
					swap(j, j+size);
					if(j == base)
						break;
				j -= size; }
			if(sp == stack)				/* No entries - done!		*/
				break;
			sp -= 2*sizeof(char*);		/* pop the base and limit	*/
			base = sp[0];
			limit = sp[1]; } }
}
