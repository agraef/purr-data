#ifndef DSPI_h
#define DSPI_h

#define DSPImin(x,y)			(((x)<(y)) ? (x) : (y))
#define DSPImax(x,y)			(((x)>(y)) ? (x) : (y))
#define DSPIclip(min, x, max)	(DSPImin(DSPImax((min), (x)), max))


// test if floating point number is denormal
#define DSPI_IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0) 

// test if almost denormal, choose whichever is fastest
#define DSPI_IS_ALMOST_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) < 0x08000000)
//#define DSPI_IS_ALMOST_DENORMAL(f) (fabs(f) < 3.e-34) 

#endif
