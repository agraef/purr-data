#include "pdoctave_dataframe.h"
//static int scheduling_ok = 1;

SharedDataFrame *newSharedDataFrame () 
{
   int id;
   SharedDataFrame *sdf;

   id = shmget (IPC_PRIVATE, sizeof(SharedDataFrame),
	    SHM_R | SHM_W);

   if (id == -1) return 0;

   sdf = shmat (id, 0, 0);

   if (sdf == (SharedDataFrame *) -1) {
      shmctl (id, IPC_RMID, 0);
      return 0;
   }

   sdf->frame_attached = 1;
   sdf->data_attached = 0;
   sdf->data_frame_id = id;
   sdf->data_vec_id = -1;
   sdf->block_for_write = 0;
   sdf->block_for_read = 0;

   return sdf;
}


SharedDataFrame *getSharedDataFrame (int id)
{
   SharedDataFrame *sdf;
   
   sdf = shmat (id, 0, 0);
   
   if (sdf == (SharedDataFrame *) -1)
      return 0;
   
   sdf->frame_attached++;
   
   return sdf;
}




void freeSharedDataFrame (SharedDataFrame **sdf)
{
   SharedDataFrame *sdfp = *sdf;
   if (sdfp) {
      if (sdfp->frame_attached > 0)
   	 sdfp->frame_attached--;
      shmdt(sdfp);
      *sdf = 0;
   }
}


void removeSharedDataFrame (SharedDataFrame **sdf)
{
   int id;
   SharedDataFrame *sdfp = *sdf;
   if (sdfp) {
      id = sdfp->data_frame_id;
      if (sdfp->frame_attached > 0)
	 shmdt (sdfp);
      if (id != -1)
	 shmctl(id, IPC_RMID, 0);
      *sdf = 0;
   }
}
      

int getSharedDataFrameId (SharedDataFrame *sdf) 
{
   if (sdf)
      return sdf->data_frame_id;
   else
      return -1;
}



DatTyp getSharedDataType (SharedDataFrame *sdf)
{
   if (sdf)
      return sdf->data_typ;
   else 
      return UNKNOWN;
}


int getSharedDataVecLength (SharedDataFrame *sdf)
{
   if (sdf)
      return sdf->data_vec_len;
   else 
      return 0;
}


void *newSharedData (SharedDataFrame *sdf, int n, int size_bytes, DatTyp dtyp)
{
   void *data;
   int id;
   
   if (sdf) {

      id = sdf->data_vec_id;
      if (id == -1) 
	 id = shmget(IPC_PRIVATE, n*size_bytes, SHM_R | SHM_W);

      if (id == -1) return 0;

      data = shmat(id,0,0);

      if (data == (void *) -1) {
	 shmctl (id, IPC_RMID, 0);
	 return 0;
      }
      
      sdf->data_attached = 1;
      sdf->data_size_bytes = size_bytes;
      sdf->data_vec_len = n;
      sdf->data_vec_id = id;
      sdf->data_typ = dtyp;

      return data;
   }
   else return 0;
}


void *getSharedData (SharedDataFrame *sdf)
{
   void *data;
   
   if ((sdf)&&(sdf->data_vec_id != -1)) {
      
      data = shmat(sdf->data_vec_id,0,0);

      if (data == (void *) -1) {
	 return 0;
      }

      sdf->data_attached++;

      return data;
   }
   else 
      return 0;
}


void freeSharedData (SharedDataFrame *sdf, void **data)
{
   void *datap = *data;
   if (datap) {
      shmdt(datap);
      *data = 0;
      if ((sdf)&&(sdf->data_attached > 0))
	 sdf->data_attached--;
   }
}


void removeSharedData (SharedDataFrame *sdf, void **data)
{
   int id;
   void *datap = *data;
   if (datap) {
      shmdt (datap);
      *data = 0;
      if (sdf) {
	 if (sdf->data_attached > 0)
	    sdf->data_attached--;
	 if (sdf->data_vec_id != -1) {
	    shmctl (sdf->data_vec_id, IPC_RMID, 0);
	    sdf->data_vec_id = -1;
	 }
      }
   }
}

void unBlockForReading (SharedDataFrame *sdf)
{
   if (sdf)
      sdf->block_for_read = 0;
}
void unBlockForWriting (SharedDataFrame *sdf) 
{
   if (sdf)
      sdf->block_for_write = 0;
}
void blockForReading (SharedDataFrame *sdf)
{
   if (sdf)
      sdf->block_for_read = 1;
}
void blockForWriting (SharedDataFrame *sdf)
{
   if (sdf)
      sdf->block_for_write = 1;
}

int sleepUntilReadUnBlocked (SharedDataFrame *sdf)
{
      int elapsed_time=0;
      while ((sdf->block_for_read!=0)&&(elapsed_time<MAX_USLEEP_TIME)) {
	   elapsed_time += STD_USLEEP_TIME;
	   usleep(STD_USLEEP_TIME);
      }
      return (elapsed_time<MAX_USLEEP_TIME);
}

int sleepUntilReadBlocked (SharedDataFrame *sdf)
{
      int elapsed_time=0;
      while ((sdf->block_for_read==0)&&(elapsed_time<MAX_USLEEP_TIME)) {
	   elapsed_time += STD_USLEEP_TIME;
	   usleep(STD_USLEEP_TIME);
      }
      return (elapsed_time<MAX_USLEEP_TIME);
}

int sleepUntilWriteUnBlocked (SharedDataFrame *sdf)
{
      int elapsed_time=0;
      while ((sdf->block_for_write!=0)&&(elapsed_time<MAX_USLEEP_TIME)) {
	   elapsed_time += STD_USLEEP_TIME;
	   usleep(STD_USLEEP_TIME);
      }
      return (elapsed_time<MAX_USLEEP_TIME);
}
int sleepUntilWriteBlocked (SharedDataFrame *sdf)
{
      int elapsed_time=0;
      while ((sdf->block_for_write==0)&&(elapsed_time<MAX_USLEEP_TIME)) {
	   elapsed_time += STD_USLEEP_TIME;
	   usleep(STD_USLEEP_TIME);
      }
      return (elapsed_time<MAX_USLEEP_TIME);
}
/*
void sleepUntilReadUnBlocked (SharedDataFrame *sdf, int usleep_time)
{
   int timer = 0;
   if (sdf) {
      while ((sdf->block_for_read!=0)&&(timer < MAX_USLEEP_TIME)) {
	 timer += usleep_time;
	 usleep (usleep_time);
      }
   }
}
void sleepUntilReadBlocked (SharedDataFrame *sdf, int usleep_time)
{
   int timer = 0;
   if (sdf) {
      while ((sdf->block_for_read==0)&&(timer < MAX_USLEEP_TIME)) {
	 timer +=usleep_time;
	 usleep (usleep_time);
      }
	 
   }
}
void sleepUntilWriteUnBlocked (SharedDataFrame *sdf, int usleep_time)
{
   int timer = 0;
   if (sdf) {
      while ((sdf->block_for_write!=0)&&(timer < MAX_USLEEP_TIME)) {
	 timer +=usleep_time;
	 usleep (usleep_time);
      }
	 
   }
}
void sleepUntilWriteBlocked (SharedDataFrame *sdf, int usleep_time)
{
   int timer = 0;
   if (sdf) {
      while ((sdf->block_for_write==0)&&(timer < MAX_USLEEP_TIME)) {
	 timer +=usleep_time;
	 usleep (usleep_time);
      }
	 
   }
}
      */
