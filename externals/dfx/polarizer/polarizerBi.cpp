/*---------------------------------------------------------------

   © 2001, Marcberg Soft und Hard GmbH, All Rights Perversed

---------------------------------------------------------------*/

#ifndef __polarizer
#include "polarizer.hpp"
#endif

#include <stdio.h>

//-----------------------------------------------------------------------------------------
// Destroy FX infos

long Polarizer::getVendorVersion() {
	return 1; }

bool Polarizer::getErrorText(char *text) {
	strcpy (text, "We're not gonna to make it.");	// max 256 char
	return true; }

bool Polarizer::getVendorString(char *text) {
	strcpy (text, "Destroy FX");	// a string identifying the vendor (max 64 char)
	return true; }

bool Polarizer::getProductString(char *text) {
	// a string identifying the product name (max 64 char)
	strcpy (text, "Super Destroy FX bipolar VST plugin pack");
	return true; }

//-----------------------------------------------------------------------------------------
Polarizer::~Polarizer()
{
	if (programName)
		delete[] programName;
}

//-----------------------------------------------------------------------------------------
void Polarizer::setProgramName(char *name)
{
	strcpy(programName, name);
}

//-----------------------------------------------------------------------------------------
void Polarizer::getProgramName(char *name)
{
	strcpy(name, programName);
}

//-----------------------------------------------------------------------------------------
void Polarizer::setParameter(long index, float value)
{
	switch (index)
	{
		case kSkip :    fSkip = value;		break;
		case kAmount :  fAmount = value;	break;
		case kImplode : fImplode = value;	break;
	}
}

//-----------------------------------------------------------------------------------------
float Polarizer::getParameter(long index)
{
	switch (index)
	{
		default:
		case kSkip :    return fSkip;
		case kAmount :  return fAmount;
		case kImplode : return fImplode;
	}
}

//-----------------------------------------------------------------------------------------
// titles of each parameter

void Polarizer::getParameterName(long index, char *label)
{
	switch (index)
	{
		case kSkip :    strcpy(label, "  leap  ");	break;
		case kAmount :  strcpy(label, "polarize");	break;
		case kImplode : strcpy(label, "implode");	break;
	}
}

//-----------------------------------------------------------------------------------------
// numerical display of each parameter's gradiations

void Polarizer::getParameterDisplay(long index, char *text)
{
	switch (index)
	{
		case kSkip :
			sprintf(text, "%ld", leapScaled(fSkip));
			break;
		case kAmount :
			sprintf(text, "%.3f", fAmount);
			break;
		case kImplode :
			if (fImplode >= 0.03f) strcpy(text, "yes");
			else strcpy(text, "no");
			break;
	}
}

//-----------------------------------------------------------------------------------------
// unit of measure for each parameter

void Polarizer::getParameterLabel(long index, char *label)
{
	switch (index)
	{
		case kSkip :
			if (leapScaled(fSkip) == 1) strcpy(label, "sample ");
			else strcpy(label, "samples");
			break;
		case kAmount :  strcpy(label, "amount");	break;
		case kImplode : strcpy(label, "      ");	break;
	}
}

//-----------------------------------------------------------------------------------------
// this is for getting the scalar value amp for the current sample

float Polarizer::processSampleAmp()
{
	switch (state)
	{
		case unaffected:	// nothing much happens in this section
			if (--unaffectedSamples <= 0)	// go to polarized when the leap is done
				state = polarized;
			return 1.0f;
			break;
			// end unaffected

		case polarized:
			state = unaffected;	// go right back to unaffected
			// but first figure out how long unaffected will last this time
			unaffectedSamples = leapScaled(fSkip);
			return (0.5f - fAmount) * 2.0f;	// this is the polarization scalar
			break;
			// end polarized

		default : return 1.0f;
	}
}

//-----------------------------------------------------------------------------------------
// this is for calculating & sending the current sample output value to the output stream

float Polarizer::processOutValue(float amp, float in)
{
  float out = (in * amp);	// if implode is off, just do the regular polarizing thing

	if (fImplode >= 0.03f)	// if it's on, then implode the audio signal
	{
		if (out > 0.0f)	// invert the sample between 1 & 0
			out = 1.0f - out;
		else	// invert the sample between -1 & 0
			out = -1.0f - out;
	}

	return out;
}
