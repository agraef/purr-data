#include "resamplesrc.h"
#include <assert.h>

#ifdef HAVE_SRC

ResampleSRC::ResampleSRC(int c)
    : Resample(c)
    , state(NULL)
{
	state = new SRC_STATE *[c];
	for(int i = 0; i < c; ++i) {
		int error;
		state[i] = src_new(SRC_ZERO_ORDER_HOLD,1,&error);
		if(!state[i]) post("src init error %i",error);
	}
}

ResampleSRC::~ResampleSRC()
{
    if(state) {
    	for(int i = 0; i < channels; ++i) src_delete(state[i]);
	    delete[] state;
    }
}

int ResampleSRC::Do(int chns,Fifo<float> *input,float *const *output,int need,double ratio)
{
    if(ratio == 1) 
        return Resample::Do(chns,input,output,frames,1);

    assert(chns <= channels);
    
    SRC_DATA src_data;
	src_data.src_ratio = ratio;
	src_data.end_of_input = 0;

    int count = -1;

	// hopefully all channel fifos advance uniformly.....
	for(int i = 0; i < chns; ++i) {
		src_set_ratio(state[i],ratio);

		for(int got = 0; got < frames; ) {
			src_data.data_out = output[i]+got;
			src_data.output_frames = frames-got;

			if(decoded[i].Have()) {
				src_data.data_in = input[i].ReadPtr();
				src_data.input_frames = input[i].ReadSamples();

				int err = src_process(state[i],&src_data);
				if(err) post("src_process error %i",err);

				// advance buffer
				decoded[i].Read(src_data.input_frames_used,NULL);
			}
			else {
				schedWait();
				if(debug) post("fifo underrun");

				// Buffer underrun!! -> zero output buffer
				memset(src_data.data_out,0,src_data.output_frames*sizeof(*src_data.data_out));
				src_data.output_frames_gen = src_data.output_frames;
			}
			got += src_data.output_frames_gen;
		}

        assert(count < 0 || got == count);
        count = got;
	}

    return count;
}

#endif
