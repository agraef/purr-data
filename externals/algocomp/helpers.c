void fast_d2bl(int x, short *c, short length) {
	int i;
	for (i=0;i<length;i++) 
	*(c++) = (x >> i) & 0x1;
}

void fast_d2b(unsigned short x, short *c) {
	int i;
	for (i=0;i<8;i++) 
	*(c++) = (x >> i) & 0x1;
}


void fast_b2d(unsigned long int *n, short *c) {
int i = 32;
*n = 0;
while(i--) {
*n <<=1;
*n+= *(c+i);
}
}

void fast_b2short8(unsigned short *n, short *c) {
int i = 8;
*n = 0;
while(i--) {
*n <<=1;
*n+= *(c+i);
}
}

void fast_b2short(unsigned int *n, short *c,short length) {
int i;
if ((length <= 16) && (length > 0))
i = length;
else i = 8;
*n = 0;
while(i--) {
*n <<=1;
*n+= *(c+i);
}
}
