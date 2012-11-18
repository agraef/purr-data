/* SdifFPut.c functions that were not declared in sdif.h*/

#include <sdif.h>
int SdifFNameValueLCurrNVTtoSdifString   (SdifFileT *SdifF, SdifStringT *SdifString);
int SdifFAllStreamIDToSdifString  (SdifFileT *SdifF, SdifStringT *SdifSTring);

/*
  Append one frame type to SdifString
*/
int SdifFOneFrameTypeToSdifString(SdifFrameTypeT *FrameType, SdifStringT *SdifString);


int SdifFOneMatrixTypeToSdifString(SdifMatrixTypeT *MatrixType, SdifStringT *SdifString);
