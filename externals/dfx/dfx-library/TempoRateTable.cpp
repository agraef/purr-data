#ifndef __TempoRateTable
#include "TempoRateTable.h"
#endif

#include <string.h>


//-----------------------------------------------------------------------------
TempoRateTable::TempoRateTable()
{
  int i;


	scalars = 0;
	scalars = new float[NUM_TEMPO_RATES];
	displays = 0;
	displays = new char*[NUM_TEMPO_RATES];
	for (i = 0; i < NUM_TEMPO_RATES; i++)
		displays[i] = new char[16];

	i = 0;
#ifdef USE_SLOW_TEMPO_RATES
	scalars[i] = 1.f/5.f;	strcpy(displays[i++], "1/12");
	scalars[i] = 1.f/6.f;	strcpy(displays[i++], "1/8");
	scalars[i] = 1.f/5.f;	strcpy(displays[i++], "1/7");
#endif
#ifndef USE_BUFFER_OVERRIDE_TEMPO_RATES
	scalars[i] = 1.f/6.f;	strcpy(displays[i++], "1/6");
	scalars[i] = 1.f/5.f;	strcpy(displays[i++], "1/5");
#endif
	scalars[i] = 1.f/4.f;	strcpy(displays[i++], "1/4");
	scalars[i] = 1.f/3.f;	strcpy(displays[i++], "1/3");
	scalars[i] = 1.f/2.f;	strcpy(displays[i++], "1/2");
	scalars[i] = 2.f/3.f;	strcpy(displays[i++], "2/3");
	scalars[i] = 3.f/4.f;	strcpy(displays[i++], "3/4");
	scalars[i] = 1.0f;		strcpy(displays[i++], "1");
	scalars[i] = 2.0f;		strcpy(displays[i++], "2");
	scalars[i] = 3.0f;		strcpy(displays[i++], "3");
	scalars[i] = 4.0f;		strcpy(displays[i++], "4");
	scalars[i] = 5.0f;		strcpy(displays[i++], "5");
	scalars[i] = 6.0f;		strcpy(displays[i++], "6");
	scalars[i] = 7.0f;		strcpy(displays[i++], "7");
	scalars[i] = 8.0f;		strcpy(displays[i++], "8");
	scalars[i] = 12.0f;		strcpy(displays[i++], "12");
	scalars[i] = 16.0f;		strcpy(displays[i++], "16");
	scalars[i] = 24.0f;		strcpy(displays[i++], "24");
	scalars[i] = 32.0f;		strcpy(displays[i++], "32");
	scalars[i] = 48.0f;		strcpy(displays[i++], "48");
	scalars[i] = 64.0f;		strcpy(displays[i++], "64");
	scalars[i] = 96.0f;		strcpy(displays[i++], "96");
#ifndef USE_SLOW_TEMPO_RATES
	scalars[i] = 333.0f;		strcpy(displays[i++], "333");
#ifndef USE_BUFFER_OVERRIDE_TEMPO_RATES
	scalars[i] = 3000.0f;	strcpy(displays[i++], "infinity");
#endif
#endif
}

//-----------------------------------------------------------------------------
TempoRateTable::~TempoRateTable()
{
	if (scalars)
		delete[] scalars;
	for (int i=0; i < NUM_TEMPO_RATES; i++)
	{
		if (displays[i])
			delete[] displays[i];
	}
	if (displays)
		delete[] displays;
	displays = 0;
}
