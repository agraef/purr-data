
/*------------------------------sdiflists-----------------------------/
  ---------------------------------------------------------------------
  An external object for Pure Data that reads SDIF files, with the help
  of IRCAM's SDIF library.  
  -written by Richie Eakin.. See the readme for more documentation.
  reakinator@gmail.com
*/

#define IFRAMES 1000
#define MAX_STREAMS 1000
#define DATE "17/1/08"
#define DEBUG 0
#define DEBUG_MEM 0

#include "m_pd.h"
#include "sdiftostring.h"
#include <sdif.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static t_class *sdiflists_class;

//a structure of outlets, decided by an init parameter.
//taken from Pd's trigger object in x_connective.c 
typedef struct colout
{
    t_symbol  u_type;        //outlet type from STYP (todo)?
    t_outlet  *out;         
    t_atom    *outvec;      
} t_colout;

// file markers, info for frame positions and times to allow seeking
typedef struct pos
{
    SdifFloat8  timetag; // seconds
    SdiffPosT   filepos; // memory locations
} t_pos;

typedef struct sdiflists
{
    t_object    x_ob;
    SdifFileT   *file ;       //file data is malloced by SDIF library and everything goes in here
    SdifStringT *string ;     //ascii queries
    t_symbol    *filename; 
    t_colout    *data;      
    int         n_outs;
    int         max_vec;      // enough memory is allocated for the biggest list that will be sent out   
    int         nframes;      //number of frames found during indexing
    t_canvas    *canvas;      //used for locating path directory
    t_atom      *infolist;    // storage for right most outlet (always info - NVT or stats)
    t_outlet    *infoout;
    t_outlet    *timeout;
    float       seconds;
    float timeRange;
    t_pos       *markers;     // locations and times of frames
    long        index;        // current frame 
    int         use_timetags; // boolean for deciding between timetag and index seeking  
    unsigned int streamid;    // a unique identifier for streams within a file
} t_sdiflists;

/*-------------------open files, STYP first if given, then SDIF-----------------*/
static void sdiflists_open(t_sdiflists *x,  t_symbol *s, int argcount, t_atom *argvec)
{	
    int i,j;
    unsigned int StreamSpecified = 0;
    
    for (i = 0; i < argcount; i++)
    {
          if (argvec[i].a_type == A_FLOAT)
          {//TODO: check if this was given after file, cuz that would be useless
                x->streamid = (int) argvec[i].a_w.w_float;
               StreamSpecified = 1;
               post("stream specified:#%d", x->streamid);
          }
 
      if (argvec[i].a_type == A_SYMBOL)
      {
          char *sym =  argvec[i].a_w.w_symbol->s_name;                 
          if(!strcmp( sym + strlen(sym)-5 , ".sdif" ) || 
                        !strcmp( sym + strlen(sym)-5 , ".SDIF" ))
          {
              t_colout *u;
              if(x->markers[0].timetag != -1) 
              {
                  post("closing file... ");
                  SdifFClose(x->file);
                  for(j=0; j < x->nframes; j++)  x->markers[j].timetag = x->markers[j].filepos = 0;
                  for(j=0, u = x->data; j < x->n_outs; u++, j++) freebytes( u->outvec, x->max_vec * sizeof(t_atom));
		 
                  x->markers[0].timetag = -1;
                  x->index = -1;
                  x->nframes = IFRAMES;
#if DEBUG                         
                  post("sdiflists::open: closed previous file");              
#endif  
	      }// end if x->file exists
              unsigned int rows = 0;
              unsigned int cols = 0;

              t_int eof, m, updatepos;
              t_int firstframe = 1;
              t_int timepos = 0;
              size_t bytesread = 0;  
              SdiffPosT currpos;
              float currtime = 0;
                  				
              /*method for opening file in canvas directory.
                Based on zexy's [msgfile], which is based on
                Pd's [textfile]*/
              char filnam[MAXPDSTRING], namebuf[MAXPDSTRING];
              char buf[MAXPDSTRING], *bufptr, *readbuf;
              int fd; // used to check if file exists
              char *dirname;
              
              dirname = canvas_getdir(x->canvas)->s_name;
              t_binbuf *bbuf = binbuf_new();
              
              fd = open_via_path(dirname, sym,"", buf, &bufptr, MAXPDSTRING, 0);
              if(fd < 0)
              {
                  error("sdiflists-open: %s cannot be found", sym);
                  return;
              }
              namebuf[0] = 0;
              if (*buf)   strcat(namebuf, buf), strcat(namebuf, "/");
              
              strcat(namebuf, bufptr);
              // open and get length 
              sys_bashfilename(namebuf, filnam);
              //this is hopefully a readable file
              
#if DEBUG
              post("(open_via_path) dirname: %s, filename->s_name: %s, buf: %s, bufptr: %s", dirname, sym, buf, bufptr);
              post("AFTER bashfilename: namebuf: %s, filnam: %s ", namebuf, filnam);
#endif
	      
              x->filename = gensym( namebuf );
	      
              /* Check if the file is a good SDIF file, skip function if not */
              if (SdifCheckFileFormat (x->filename->s_name))
              { 
                  post("sdiflists: reading %s", x->filename->s_name);
                  x->file = SdifFOpen ( x->filename->s_name, eReadFile); 
     		  bytesread += SdifFReadGeneralHeader  (x->file);       
                  bytesread += SdifFReadAllASCIIChunks (x->file);                 
                  eof = SdifFCurrSignature(x->file) == eEmptySignature;	
                  int err;
                  while (!eof) //frame loop
                  {
                      /*The frame positions must be indexed before the
                        frameheader is read, then check if it is a
                        selected frame.  If not, skip the frame and
                        overwrite the marker.*/
                      err = SdifFGetPos(x->file, &currpos);
                      if(err==-1) error("error SdifFGetPos");
						
						
                      /* Read frame header.  Current signature has already been read
                         by SdifFReadAllASCIIChunks or the last loop.) */
                      bytesread += SdifFReadFrameHeader (x->file);

                      if(!StreamSpecified)
                        {
                          x->streamid = SdifSelectGetFirstInt(x->file->Selection->stream, SdifFCurrID (x->file));
                          StreamSpecified = 1;
                          post("first stream used: #%d", x->streamid);
                        }

                      //PROBLEM: (maybe not...check)the last frame in the file is always acceptable... fix by using sel spec			
                      while (!SdifFCurrFrameIsSelected (x->file) ||  SdifFCurrID (x->file) != x->streamid )
                      {
	   //  post("frame skipped");
                          SdifFSkipFrameData (x->file);
                          if ((eof = SdifFGetSignature(x->file, &bytesread) == eEof)) break;  
                          SdifFGetPos(x->file, &currpos);			
                          bytesread += SdifFReadFrameHeader(x->file);			
                       }

                      if(eof) //have to check again...since it might have skipped to the end
                        break;
			
                      //check if this is a new time so successive frames don't overwrite filepos
                      currtime =  SdifFCurrTime (x->file);
                      if( !timepos ||  x->markers[timepos-1].timetag  != currtime )
                      {
                            x->markers[timepos].filepos = currpos;
                            x->markers[timepos].timetag  = currtime;
                            timepos++;
                            if( timepos >= x->nframes )
                            {
                                   x->markers = (t_pos *)resizebytes( x->markers, x->nframes * sizeof(t_pos),
		                               (x->nframes + IFRAMES) * sizeof(t_pos) );                                        
                                   x->nframes = x->nframes + IFRAMES;
                             }
                      }      						
                      /*matrices loop */
                      for ( m = 0; (unsigned int)m < SdifFCurrNbMatrix (x->file); m++)
                      {  
                          bytesread += SdifFReadMatrixHeader (x->file);
			  
                          if( SdifFCurrNbRow (x->file) > rows)
                          {
                              rows = SdifFCurrNbRow (x->file); //get matrix stats
                              cols = SdifFCurrNbCol (x->file);//should stay the same
                           }
                          //skip the actual matrices
                          bytesread += SdifFSkipMatrixData (x->file);
                      }// end for matrices
                      eof = SdifFGetSignature (x->file, &bytesread) == eEof; 
						
                 }// end while no eof
                 x->seconds = currtime;
                 x->nframes= timepos; //last timepos was the eof
#if DEBUG
                  post(" rows: %d , cols: %d , frames: %d , seconds: %f", rows, cols,  x->nframes, x->seconds);
                  post("sdiflists: %s opened. ", x->filename->s_name);
#endif
                  x->max_vec = rows; //needed to free memory
	 
                  //the following lines are a bug work-around... if the eof is reached, you cannot seek
                  //until the fle is closed, opened, and re-initialized... i posted about this on the sdif list.
                  SdifFClose (x->file);
                  x->file = SdifFOpen ( x->filename->s_name, eReadFile); 
                  SdifFReadGeneralHeader  (x->file); 
                  SdifFReadAllASCIIChunks (x->file); 
                  eof = SdifFCurrSignature(x->file) == eEmptySignature;  
		
                  //make room for the row lists
                  for ( i=0, u = x->data; i < x->n_outs; u++, i++)
	   u->outvec = (t_atom *)getbytes( x->max_vec * sizeof(t_atom));
	     
               } /* end if filetype check */
            }
        } //end if Symbol
    }//end for arguments
}

/*--------------------seek to position and output data---------------------*/
/*SDIF selection spec is infile::[#stream][:frame][/matrix][.column][_row][@time]*/
/*in any order (accept :: is always first), allowing spaces*/
static void sdiflists_seek(t_sdiflists *x, t_floatarg gotoindex)
{
  //check if file has been opened, -1 means first timetag was never set
  if(x->markers[0].timetag == -1) 
    {
      post("sdiflists: open a file before seeking");
      return;
    }
  x->index = (long)gotoindex;
  //check if index  exists 
  if( x->index >= x->nframes || gotoindex < 0)
    {
       
#if DEBUG
      post("no index there.");
#endif
      return;
    }

#if DEBUG_MEM
  post ("index = %d , file pos = %d", x->index, x->markers[x->index].filepos);
#endif
    
  t_colout *u;
  int      i, selectedrow, selectedcol;
  unsigned int m;
  int      nbrows =0;
  int      nbcols =0;
  int      xout = 0;
  int      eof = 0;
  int      col, row;
  float   value = 0;
  size_t   bytesread;
  int       listsize[x->n_outs];
  float timetag;
  
  //seek to index
  SdifFSetPos( x->file , &x->markers[x->index].filepos);
  
  //loop through 
  while(!eof)
  {
      SdifFReadFrameHeader(x->file);
      timetag =   SdifFCurrTime (x->file);	
      //break once the current time has changed
      if( x->markers[x->index].timetag != timetag) break;
 
      outlet_float(x->timeout, timetag);

      /*Get the Data.  If no selection is given when the file is opened,
        all rows/columns/matrices/frames will be obtained. */
      if(SdifFCurrFrameIsSelected(x->file))
      {   
         
#if DEBUG
          //fsig turns up 0 unless I close file and reopen in in the open function..
          SdifSignature   fsig     = SdifFCurrFrameSignature (x->file);
          SdifUInt4       streamid = SdifFCurrID (x->file);
          post("\n frame header- Signature: '%s', timetag: %f, streamid: %d,  nmatrices: %d", 
               SdifSignatureToString(fsig),x->markers[x->index].timetag, streamid,SdifFCurrNbMatrix (x->file));
#endif
 
          //each frame can contain multiple matrices, so go through all of them
          for( m= 0; m < SdifFCurrNbMatrix (x->file) ; m++)
          {
              bytesread =  SdifFReadMatrixHeader(x->file); 
              //matrices not selected will be skipped, but they still need to be added to the bytesread tally			
              if (SdifFCurrMatrixIsSelected (x->file)) 
              {
                  //get how many rows total, then check which ones are selected
                  nbrows = SdifFCurrNbRow (x->file);
                  nbcols = SdifFCurrNbCol (x->file);
#if DEBUG
                  //same problem with matrix signature as frame signature
                  SdifDataTypeET  type = 	SdifFCurrDataType(x->file);		
                  post("matrix header: Signature: '%s', nbrows: %d,  nbcols %d,  type %04x", 
                  SdifSignatureToString(SdifFCurrSignature(x->file)), nbrows, nbcols, type);
#endif

                  //go through every row and build vectors according to which column the row i sin
                  for (selectedrow = 0, row = 0; row < nbrows; row++)
                  {
                      if(SdifFRowIsSelected (x->file, row))
                      {
                          bytesread +=  SdifFReadOneRow (x->file);                          
                          //put each member in the correct vector by accessing each one according to the current column
                          //plus any previous columns from other matrices/frames         
                          for ( selectedcol= 0, col = 1, u = x->data + xout + selectedcol;
                                col <= nbcols &&  xout + selectedcol < x->n_outs; col++, u++)   
                          {
                                  
                              if(SdifFColumnIsSelected (x->file, col))
                              {
                                  value = SdifFCurrOneRowCol (x->file, col);
                                  SETFLOAT(u->outvec + selectedrow, value);
                                  selectedcol++;
                              }//end if col is selected	
                          } //end for cols
                          selectedrow++;
                        }//end if row is selected
                  }//end for rows
                  //set how big each vector is that was just filled
                  for(i = 0; i < SdifFNumColumnsSelected (x->file); i++)
                    {
                      listsize[xout + i] =  SdifFNumRowsSelected (x->file);
                    }
                  //update number of columns
                  xout += SdifFNumColumnsSelected (x->file);
                }//end if matrix is selected
              else   bytesread += SdifFSkipMatrixData(x->file);
             
              SdifFReadPadding(x->file, SdifFPaddingCalculate(x->file->Stream, bytesread));
            }//end for matrices in frame
        }// end if  current frame is selected and not eof
       else   SdifFSkipFrameData(x->file);
 
       eof = SdifFGetSignature (x->file, &bytesread) == eEof;             
   } //end while not eof

  /* !! Once eof has been found, I have to close the file and reopen for the
     frame signatures to be found.  someone please tell me why this is.. */
  if(eof)
  {
      SdifFClose (x->file);
      x->file = SdifFOpen ( x->filename->s_name, eReadFile); 
      SdifFReadGeneralHeader  (x->file); 
      SdifFReadAllASCIIChunks (x->file); 
      eof = SdifFCurrSignature(x->file) == eEmptySignature;  
      
  }
#if DEBUG
  if (SdifFLastError (x->file))   
  {
      error(" in sdiflists_seek() ");
  } 
#endif
  // send out columns of rows, preserving right to left order 
for (i = x->n_outs, u = x->data + i; u--, i--;)
      outlet_list(u->out, gensym("list"),listsize[i], u->outvec);
}

static void sdiflists_timeRange(t_sdiflists *x, t_floatarg t)
{
    x->timeRange = t;
}
static void sdiflists_time(t_sdiflists *x, t_floatarg time)
{
  int i;
  float timeDiff;
  for( i = 0; i < x->nframes; i++)
  {

      timeDiff = fabs( x->markers[i].timetag - time );
      if(timeDiff < x->timeRange ) 
      {
          sdiflists_seek(x, i);
          break;
      }
  }
}

static void sdiflists_bang(t_sdiflists *x)
{
  sdiflists_seek(x, (float)++x->index);
}


/*--------------------------------------------------------*/
/*	query posts frame, matrix, and all ascii info	  */
static void sdiflists_info(t_sdiflists *x)
{

  /* What I want to Post:
     - information about external
     - filename
     - 1NVT
     - streams in file, frame/matrix types
     - selection
     - column info
     - framecount and timerange
  */

  post("\n_-=-_ sdiflists -info _-=-_");
  
  if(x->markers[0].timetag == -1) 
    {
      post("sdiflists: open a file before getting info");
      return;
    }
  post("filename: %s", x->filename->s_name);


  //size_t bytesread = 0; 
  //int eof; 


  SdifFClose (x->file);
  x->file = SdifFOpen ( x->filename->s_name, eReadFile); 
  SdifFReadGeneralHeader  (x->file); 
  SdifFReadAllASCIIChunks (x->file); 
  


  //----send all Names Values Tables out right most list outlet----
  if (SdifNameValuesLIsNotEmpty(x->file->NameValues)) 
  {
      x->string = SdifStringNew();
      SdifListInitLoop(x->file->NameValues->NVTList); 
      while (SdifListIsNext(x->file->NameValues->NVTList)) 
      {
          x->file->NameValues->CurrNVT = (SdifNameValueTableT *)
	      SdifListGetNext(x->file->NameValues->NVTList);
	  
          SdifUInt4       iNV;
          SdifHashNT     *pNV;
          SdifHashTableT *HTable;
          HTable = x->file->NameValues->CurrNVT->NVHT;
	  
          for(iNV=0; iNV<HTable->HashSize; iNV++)
	      for (pNV = HTable->Table[iNV]; pNV; pNV = pNV->Next)
              {
		  SdifNameValueT *NameValue = (SdifNameValueT *) pNV->Data;
		  SETSYMBOL(x->infolist , gensym(NameValue->Name));
		  SETSYMBOL(x->infolist + 1 , gensym(NameValue->Value));
		  outlet_list(x->infoout, gensym("list"), 2 , x->infolist);
              }//end for pNV
      } //end list while NVT
      SdifStringFree(x->string);
  }// end if NVT is not empty

  //---------------------- post 1IDS ---------------------------
  if (   (SdifExistUserMatrixType(x->file->MatrixTypesTable))
         || (SdifExistUserFrameType(x->file->FrameTypesTable)) )
    {
      x->string = SdifStringNew();
      if ((x->file->TypeDefPass == eNotPass) || (x->file->TypeDefPass == eReadPass))
        {   
          SdifFAllMatrixTypeToSdifString(x->file, x->string); 
          SETSYMBOL ( x->infolist , gensym( SdifSignatureToString(e1TYP)) );
          SETSYMBOL ( x->infolist + 1 , gensym(x->string->str) );
          outlet_list(x->infoout, gensym("list"), 2 , x->infolist);

          post("----  %s  ----",SdifSignatureToString(e1TYP));
				
	  SdifFAllFrameTypeToSdifString(x->file, x->string);		//this compiles
	}
      SdifStringFree(x->string);	
    }

  if (SdifStreamIDTableGetNbData  (x->file->StreamIDsTable) > 0)
  {
      x->string = SdifStringNew();
      if ((x->file->StreamIDPass == eNotPass) || (x->file->StreamIDPass == eReadPass))
      {      
	  SdifFAllStreamIDToSdifString(x->file, x->string);	//this also undeclared in sdif.h. x->string?	
	  SETSYMBOL ( x->infolist , gensym(SdifSignatureToString(e1IDS)) );
          SETSYMBOL ( x->infolist + 1 , gensym(x->string->str) );
          outlet_list(x->infoout, gensym("list"), 2 , x->infolist);

      }
      SdifStringFree(x->string);	
  }
			
  //------ output Stream ID's and corresponding Frame types --------
  size_t  bytesread = 0;
  int eof = 0;
  int nStream = 0;
  int streamIDlist[MAX_STREAMS];
  int j, newStream;
  for(j = 0; j < MAX_STREAMS; j++)
      streamIDlist[j] = -1;
  while(!eof)
  {
      bytesread += SdifFReadFrameHeader (x->file);
      SdifUInt4       streamid = SdifFCurrID (x->file);
      SdifSignature   fsig     = SdifFCurrFrameSignature (x->file);
      newStream = 1;
      for(j = 0; j < MAX_STREAMS; j++)
      {
	  if(streamIDlist[j] == (signed) streamid)
	      newStream = 0;
      }
      if(newStream)
      {
	  streamIDlist[nStream++] = streamid;
	  SETSYMBOL(x->infolist , gensym("stream"));
	  SETFLOAT(x->infolist + 1, streamid );
	  outlet_list(x->infoout, gensym("list"), 2 , x->infolist);

	  SETSYMBOL(x->infolist , gensym("frametype"));
	  SETSYMBOL (x->infolist + 1, gensym(SdifSignatureToString(fsig)) );
	  outlet_list(x->infoout, gensym("list"), 2 , x->infolist);
      }

      bytesread += SdifFSkipFrameData (x->file);
      eof = (SdifFGetSignature(x->file, &bytesread)== eEof);
 
  }
  
  //also send other useful info out rightmost list outlet
  SETSYMBOL(x->infolist , gensym("frames"));
  SETFLOAT (x->infolist + 1, (float) x->nframes);
  outlet_list(x->infoout, gensym("list"), 2 , x->infolist);
    
  SETSYMBOL(x->infolist , gensym("seconds"));
  SETFLOAT (x->infolist + 1, x->seconds );
  outlet_list(x->infoout, gensym("list"), 2 , x->infolist);
	 
  SETSYMBOL(x->infolist , gensym("maxlist"));
  SETFLOAT (x->infolist + 1, x->max_vec );
  outlet_list(x->infoout, gensym("list"), 2 , x->infolist);
   
 
}		

/*--------------------------------------------------------*/
/* about for author info */
static void sdiflists_about(t_sdiflists *x)
{
  post("~~~ [sdiflists] v0.1 ~~~");
  post("by Richie Eakin, reakinator@gmail.com");
  post("source last edited: %s", DATE);
  post("SDIF Ircam library version %s \n", SDIF_VERSION_STRING ); 
}
						
static void sdiflists_error(t_sdiflists *x, t_floatarg bool)
{	
  if(bool == 0) 
    {
      SdifDisableErrorOutput ();
      post("error output disabled.");
    }

  if(bool == 1)
    {
      SdifEnableErrorOutput  ();	
      post("error output enabled.");
    }
}

static void sdiflists_types(t_sdiflists *x, t_symbol *types)
{
                  
  if(x->markers[0].filepos != -1 ) 
    {
      t_colout *u;
      int j;
      SdifFClose(x->file);
      for(j=0; j < x->nframes; j++)
        x->markers[j].timetag = x->markers[j].filepos = 0;

      x->markers[0].timetag = -1;
   
      for ( j=0, u = x->data; j < x->n_outs; u++, j++)
        freebytes( u->outvec, x->max_vec * sizeof(t_atom));
  
#if DEBUG                         
      post("sdiflists::types: closed previous file");              
#endif  
      x->nframes = IFRAMES;
    }                     	
  //prepend the full path onto the filename
  char fullfilename[MAXPDSTRING], namebuf[MAXPDSTRING];
  char buf[MAXPDSTRING], *bufptr, *dirname;
  int fd = 0;
															
  dirname=canvas_getdir(x->canvas)->s_name;
  t_binbuf *bbuf = binbuf_new();
  fd = open_via_path(dirname,  types->s_name,"", buf, &bufptr, MAXPDSTRING, 0);
  if(fd > 0)
    {
      namebuf[0] = 0;
      if (*buf)  
        strcat(namebuf, buf), strcat(namebuf, "/");
      strcat(namebuf, bufptr);
      sys_bashfilename(namebuf, fullfilename);
               
      SdifGenKill (); // have to close it to read a new .STYP
      SdifGenInitCond (types->s_name);
               
      post("sdiflists: types-declaration file: %s", types->s_name);     
    }
  else post ("sdiflists: types file %s could not be opened", types->s_name);
}



/*--------------------constructors and destructors---------------------*/
/* optional creation arguments are typechecked for number of columns,
   STYP types-definition file, and initial SDIF file. The STYP should be
   first so that the SDIF file is defined correctly.*/

/*TODO: -c x or -m y to decide between columns or matrices output*/
static void *sdiflists_new(t_symbol *s, int argcount, t_atom *argvec)
{
 
#if DEBUG 
  post("~~~ sdiflists: Debug Mode ~~~");
 
#endif

  
  t_sdiflists *x = (t_sdiflists *)pd_new(sdiflists_class);

  x->canvas = canvas_getcurrent();
  t_colout *u;

  int i, j;
  int gotcols = 0;

  x->n_outs = 0;
  x->nframes = IFRAMES;
  x->timeRange = 0.01;
  x->index = -1; // will be incremented to 0 if banged, reset if seeked

 

  SdifGenInitCond (""); /*initialize standard .STYP types file if uninitialized */

  for (i = 0; i < argcount; i++)
    {
	
      if (argvec[i].a_type == A_FLOAT)
        {
			 gotcols= 1;
			 x->n_outs = (int) argvec[i].a_w.w_float;
			 x->data = (t_colout *)getbytes(x->n_outs * sizeof(*x->data));
                

          /*number of outlets is defined here; one for each column specified or
            one for each matrix (if x->n_outs is zero) */
          for ( j=0, u = x->data; j < x->n_outs; u++, j++)
            {
              u->out = outlet_new (&x->x_ob, &s_list);
            }
        }
      if (argvec[i].a_type == A_SYMBOL)
        {
          t_atom at;
          SETSYMBOL(&at , argvec[i].a_w.w_symbol );
          char *sym =  argvec[i].a_w.w_symbol->s_name;
                
          if( (!strcmp( sym + strlen(sym)-5 , ".sdif" ) || 
               !strcmp( sym + strlen(sym)-5 , ".SDIF" )) && gotcols )  	
            {	   
              sdiflists_open(x, 0, 1, &at);
            }
          if(!strcmp( sym + strlen(sym)-5 , ".styp" ) || 
             !strcmp( sym + strlen(sym)-5 , ".STYP" )) 	
            {
              sdiflists_types(x, argvec[i].a_w.w_symbol);
            }
			    
        }//end if symbol argument
          
		
         
    }//end for creation arguments

  x->timeout =outlet_new(&x->x_ob, &s_float);
  x->infoout = outlet_new(&x->x_ob, gensym("list"));
  x->markers = (t_pos *)getbytes( x->nframes * sizeof(t_pos));
  x->infolist = (t_atom *)getbytes( 2 * sizeof(t_atom));
  x->markers[0].timetag = -1; //if this isn't -1, a file is open	
  return (void *)x;	
}

/*cleanup function - leave SDIF library in case there are other files using it */
static void sdiflists_free(t_sdiflists *x)
{
  t_colout *u;
  int i;
  SdifFClose (x->file);
   
  for ( i=0, u = x->data; i < x->n_outs; u++, i++)
    freebytes( u->outvec, x->max_vec * sizeof(t_atom));
  
  freebytes(x->data, x->n_outs * sizeof(*x->data));
  freebytes(x->markers, x->nframes * sizeof(t_pos));
  freebytes(x->infolist, 2 * sizeof(t_atom));
}

/*SETUP: a float inlet and variable creation arguments. */
void sdiflists_setup(void)
{
  sdiflists_class = class_new(gensym("sdiflists"), (t_newmethod)sdiflists_new, (t_method)sdiflists_free,sizeof(t_sdiflists), 0, A_GIMME, 0);
  //open - filename/streams input(to do: multiple streams)
  class_addmethod(sdiflists_class, (t_method)sdiflists_open, gensym("open"), A_GIMME , 0);
  //seek - float is used as index or (TODO) time
  class_addfloat(sdiflists_class, (t_method)sdiflists_seek);
  // bang - sends out next frame 
  class_addbang(sdiflists_class, sdiflists_bang);
  //info - print statistics and useful information stored in file
  class_addmethod(sdiflists_class, (t_method)sdiflists_info, gensym("info"), 0);
  //error - disable/re-enable error messages from sdif library
  class_addmethod(sdiflists_class, (t_method)sdiflists_error, gensym("error"), A_DEFFLOAT);
  //get -- get info and send out right most outlet
  class_addmethod(sdiflists_class, (t_method)sdiflists_about, gensym("about"), 0);
  //types - change the STYP file from default ( SdifTypes.STYP)
  class_addmethod(sdiflists_class, (t_method)sdiflists_types, gensym("types"), A_DEFSYM, 0);
  //time - specify desired frame by time intead of frame number
  class_addmethod(sdiflists_class, (t_method)sdiflists_time, gensym("time"), A_DEFFLOAT, 0);
  //timerange - the timestamp selected is within this amount of the time specification
  class_addmethod(sdiflists_class, (t_method)sdiflists_timeRange, gensym("timerange"), A_DEFFLOAT, 0);
}
