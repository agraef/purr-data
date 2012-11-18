//////////////////////////////////////////////////
// BufSyncGrain implementation      
// VL, 2002
// Changed to use buftable, fbar 2003
//////////////////////////////////////////////////

#include "BufSyncGrain.h"

BufSyncGrain::BufSyncGrain(){

   m_table = 0;       // wavetable
   m_envtable = 0;    // envelope table

   m_amp = 1.f;    // overall amp
   m_inputamp =0;
   m_fr = 440.f;     // fundamental freq
   m_inputfr = 0;
   m_pitch = 1.f;  // grain pitch
   m_inputpitch = 0;
   m_grsize = 0.f; // size of grains (msecs)
   m_inputgrsize = 0;
   m_olaps = 100;  // max number of streams (overlaps)

   m_point = 0;

   if(!(m_index = new float[m_olaps])){
	   m_error = 11;
	   return;  // index into wavetable
   }
   if(!(m_envindex = new float[m_olaps])){
	   m_error = 11;
	   return;
   } // index into envtable
   if(!(m_streamon = new short[m_olaps])){
	   m_error = 11;
	   return;
   }

   m_count = 0;    // sampling period counter
   m_numstreams = 0;  // curr num of streams
   m_firststream = 0; // streams index (first stream) 
   m_tablesize = 0; // size of wavetable
   m_envtablesize = 0; // size of envtable
   
   for(int i = 0; i < m_olaps; i++) {
	   m_streamon[i] = 0;
	   m_index[i] = m_envindex[i] = 0.f;
   }
      m_start = 0.f;
	  m_frac = 0.f;
}

BufSyncGrain::BufSyncGrain(buftable* wavetable, Table* envtable, float fr, float amp,
	          float pitch, float grsize, float prate, SndObj* inputfr, 
			  SndObj* inputamp, SndObj* inputpitch, 
			  SndObj* inputgrsize, int olaps,
					   int vecsize, float sr):
SndObj(inputfr, vecsize, sr){

   m_table = wavetable;       // wavetable
   m_envtable = envtable;    // envelope table

   m_amp = amp;    // overall amp
   m_inputamp = inputamp;
   m_fr = fr;     // fundamental freq
   m_inputfr = inputfr;
   m_pitch = pitch;  // grain pitch
   m_inputpitch = inputpitch;
   m_grsize = grsize; // size of grains (msecs)
   m_inputgrsize = inputgrsize;
   m_olaps = olaps;  // max number of streams (overlaps)

   if(!(m_index = new float[m_olaps])){
	   m_error = 11;
	   return;  // index into wavetable
   }
   if(!(m_envindex = new float[m_olaps])){
	   m_error = 11;
	   return;
   } // index into envtable
   if(!(m_streamon = new short[m_olaps])){
	   m_error = 11;
	   return;
   }

   m_count = 0xFFFFFFFF;    // sampling period counter
   m_numstreams = 0;  // curr num of streams
   m_firststream = 0; // streams index (first stream) 
   m_tablesize = m_table->GetLen(); // size of wavetable
   m_envtablesize = m_envtable->GetLen(); // size of envtable

      for(int i = 0; i < olaps; i++){
	   m_streamon[i] = 0;
	   m_index[i] = m_envindex[i] = 0.f;
   }
   m_start = 0.f;
   m_point = prate;
   m_frac = 0.f;
}

BufSyncGrain::~BufSyncGrain(){

   // perform any necessary de-allocation etc
   // here
   delete[] m_index;
   delete[] m_envindex;
   delete[] m_streamon;

}

short
BufSyncGrain::DoProcess(){

if(!m_error){

	 float sig, pitch, amp, grsize, envincr, period;
	 for(m_vecpos = 0; m_vecpos < m_vecsize; m_vecpos++) {
		if(m_enable) {
        
	// set the control parameters 
	// (amp, fund period, grain pitch and size, in samples)
         sig = 0.f; 
		 pitch  = m_pitch + (m_inputpitch ? m_inputpitch->Output(m_vecpos) : 0); 
         period = m_frac + m_sr/(m_fr + (m_inputfr ? m_inputfr->Output(m_vecpos) : 0));
         amp = m_amp + (m_inputamp ? m_inputamp->Output(m_vecpos) : 0); 
		 grsize =  (m_sr *
			 (m_grsize + (m_inputgrsize ? m_inputgrsize->Output(m_vecpos) : 0)));
         envincr = m_envtablesize/grsize;
   

  // if a grain has finished, clean up
  if((!m_streamon[m_firststream]) && (m_numstreams) ){
   m_numstreams--; // decrease the no of streams
   m_firststream=(m_firststream+1)%m_olaps; // first stream is the next
  }  
  
   // if a fund period has elapsed
   // start a new grain
  if(m_count >= period){
   m_frac = m_count - period; // frac part to be accummulated
   int newstream =(m_firststream+m_numstreams)%m_olaps;
   m_streamon[newstream] = 1; // turn the stream on
   m_envindex[newstream] = 0.f;   
   m_index[newstream] = m_start;
   m_numstreams++; // increase the stream count
   m_count = 0;  
   m_start += m_point*grsize;
   while (m_start > m_tablesize) m_start-=m_tablesize;
  }
			 
   for(int i=m_numstreams, 
	   j=m_firststream; i; i--, j=(j+1)%m_olaps){
              	   
			  // modulus
	      while(m_index[j] > m_tablesize) 
				  m_index[j] -= m_tablesize;
			  while(m_index[j] < 0)
				  m_index[j] += m_tablesize;

	  // sum all the grain streams
      sig += m_table->GetTable()[(int)m_index[j]]
		  *m_envtable->GetTable()[(int)m_envindex[j]];

	          // increment the indexes
	          // for each grain
              m_index[j] += pitch;
              m_envindex[j] += envincr;
			  
			  // if the envelope is finished
			  // the grain is also finished

			  if(m_envindex[j] > m_envtablesize)			  
				  m_streamon[j] = 0;
			 }

        // increment the period counter
        m_count++; 
		// scale the output
        m_output[m_vecpos] = sig*amp;
			
		}
     else m_output[m_vecpos] = 0.f;
	 } 
 return 1; 
}
else return 0;
}

char*
BufSyncGrain::ErrorMessage(){
 
  char* message;
   
  switch(m_error){

  // handle your error codes here 
  case 11:
  message = "Memory allocation error\n";
  break;

  default:
  message = SndObj::ErrorMessage();
  break;
  
  }

 return message;

}
