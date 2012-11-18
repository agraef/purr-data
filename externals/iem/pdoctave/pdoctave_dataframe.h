#ifndef __pdoctave_data_frame_h__
#define __pdoctave_data_frame_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <stdio.h>

#include "pdoctave_datatypes.h"

#define STD_USLEEP_TIME 100
#define MAX_USLEEP_TIME 5000000

typedef struct _SharedDataFrame_ SharedDataFrame;

struct _SharedDataFrame_ {
   int data_frame_id;

   int data_vec_id;
   int data_vec_len;
   int data_size_bytes;
   DatTyp data_typ;

   // internal states
   int frame_attached;
   int data_attached;

   int block_for_read;
   int block_for_write;
};

SharedDataFrame *newSharedDataFrame ();
SharedDataFrame *getSharedDataFrame (int id);
void freeSharedDataFrame (SharedDataFrame **sdf);
void removeSharedDataFrame (SharedDataFrame **sdf);
int getSharedDataFrameId (SharedDataFrame *sdf);

DatTyp getSharedDataType (SharedDataFrame *sdf);
int getSharedDataVecLength (SharedDataFrame *sdf);

void *newSharedData (SharedDataFrame *sdf, int n, int size_bytes, DatTyp dtyp);
void *getSharedData (SharedDataFrame *sdf);
void freeSharedData (SharedDataFrame *sdf, void **data);
void removeSharedData (SharedDataFrame *sdf, void **data);

void unBlockForReading (SharedDataFrame *sdf);
void unBlockForWriting (SharedDataFrame *sdf);
void blockForReading (SharedDataFrame *sdf);
void blockForWriting (SharedDataFrame *sdf);
int sleepUntilReadUnBlocked (SharedDataFrame *sdf);
int sleepUntilReadBlocked (SharedDataFrame *sdf);
int sleepUntilWriteUnBlocked (SharedDataFrame *sdf);
int sleepUntilWriteBlocked (SharedDataFrame *sdf);
/*
void sleepUntilReadUnBlocked (SharedDataFrame *sdf, int usleep_time);
void sleepUntilReadBlocked (SharedDataFrame *sdf, int usleep_time);
void sleepUntilWriteUnBlocked (SharedDataFrame *sdf, int usleep_time);
void sleepUntilWriteBlocked (SharedDataFrame *sdf, int usleep_time);
*/


#ifdef __cplusplus
}
#endif

#endif
