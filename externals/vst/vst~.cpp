#include "stdafx.h"
#include "m_pd.h"
#include <io.h>
#include <stdlib.h>
#include <direct.h>
#include<time.h>
#include "EditorThread.h"
#include "VstHost.h"

#include "vst~.h"

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
extern "C" {
#endif

static const char * findFilePath( const char * path , const char * dllname );

/* ------------------------ vst_tilde~ ----------------------------- */

static t_class *vst_tilde_class;

/**
*	the perform routine unpacks its parameters 
*	looks to see if time is zero (do channel prob 
*	everytime) if it is to chooses a channel.
*	the routine then copies everything in the input
*	to the choosen output
*/

t_int *vst_tilde_perform_replace(t_int *w)
{
     t_dsp_args *args = (t_dsp_args *)(w[1]);
    int n = (t_int)(w[2]);	
	args->plug->processReplacing( args->inbufs , args->outbufs , args->num_samples );
   return w+3;
}

t_int *vst_tilde_perform_acc(t_int *w)
{
     t_dsp_args *args = (t_dsp_args *)(w[1]);
    int n = (t_int)(w[2]);
	args->plug->process( args->inbufs , args->outbufs , args->num_samples );
   return w+3;
}

/**
*	set up our dsp perform routine - it takes parameters
*	the input channel, the output channels ( left and right), 
*	the pin object and the number of samples in the array
*/

static void vst_tilde_dsp(t_vst_tilde *x, t_signal **sp)
{
	int i;	
	if ( x->plug != NULL )
	{
		x->d_args = (t_dsp_args *) malloc( sizeof( t_dsp_args ));
		x->d_args->num_in = x->num_audio_inputs;
		x->d_args->num_out = x->num_audio_outputs;
		x->d_args->inbufs = (float**) malloc( x->d_args->num_in * sizeof( float *));
		for( i = 0 ; i< x->d_args->num_in;i++ )
		{
			x->d_args->inbufs[i] = sp[ i ]->s_vec;
		}
		x->d_args->outbufs = (float**) malloc( x->d_args->num_out * sizeof( float *));
		for( i = 0 ; i< x->d_args->num_out;i++ )
		{	
			x->d_args->outbufs[i] = sp[ i + x->d_args->num_in + 1]->s_vec;
		}
		x->d_args->num_samples = sp[0]->s_n;
		x->d_args->sample_rate = sp[0]->s_sr;
		x->d_args->plug = x->plug;
		//post("*");
		x->plug->Init( sp[0]->s_sr  , sp[0]->s_n );	
		//post("*");
		if ( x->plug->replace() )
		{
			dsp_add(vst_tilde_perform_replace, 2 , x->d_args , sp[0]->s_n );
		}
		else
		{
			dsp_add(vst_tilde_perform_acc, 2 , x->d_args , sp[0]->s_n );
		}
	}

}

/**
*	free up the tilde object - for now we only need 
*	to get rid of the clock
*/

static void vst_tilde_free(t_vst_tilde *x)
{	
	int i;
  /* Destroy inlets */
    if (x->audio_inlets != NULL) 
	{	
		for (i = 0; i < x->num_audio_inputs; i++) 
		{
			inlet_free (x->audio_inlets[i]);
		}
		free (x->audio_inlets);
		x->audio_inlets = NULL;
    }
    /* Destroy outlets */
    if (x->control_outlet != NULL) 
	{
		outlet_free (x->control_outlet);
		x->control_outlet = NULL;
	}
    if (x->audio_outlets != NULL) 
	{
		for (i = 0; i < x->num_audio_outputs; i++) 
		{
			outlet_free (x->audio_outlets[i]);
		}
		free (x->audio_outlets);
		x->audio_outlets = NULL;
    }

	if( x->d_args != NULL)
	{
		free( x->d_args->inbufs );
		free( x->d_args->outbufs);
		free( x->d_args );
	}
	if ( x->plug != NULL )
	{
		delete x->plug ;
	}
}

/**
*	make a new object - set up out internal variables 
*	and add our inlets and outlets
*/

static void *vst_tilde_new( t_symbol *s, int argc, t_atom *argv)
{
	post("In vst~ new");
	t_vst_tilde *x = (t_vst_tilde *)pd_new(vst_tilde_class);
	x->d_args = NULL;
	x->plug = new VSTPlugin();
	 x->audio_inlets = NULL;
	  x->audio_outlets = NULL;
	  x->control_outlet = NULL;

	// to help deal with spaces we assume ALL of the args make 
	// up the filename 
	char buf[255];	
	CString str;
	for( int i = 0 ; i < argc ; i++ )
	{
		atom_string( &argv[i] , buf, 255 );
		if ( i == 0 )
		{
			str += buf;
		}
		else
		{
			str += ' ';
			str += buf;
		}
	}
	bool lf = FALSE;

	// try loading the dll from the raw filename 
	if ( x->plug->Instance(  str ) == VSTINSTANCE_NO_ERROR )
	{
		//post( "it loaded fine ");
		lf = TRUE;
	}
	else // try finding it on the VST PAth
	{
		
		char* vst_path = getenv ("VST_PATH");
		char* tok_path = (char *) malloc( strlen( vst_path) * sizeof( char ));

		CString dllname;
		if ( str.MakeLower().Find( ".dll" ) == -1 )
		{
			dllname = str + ".dll";			
		}
		else
		{
			dllname = str;
		}
		strcpy( tok_path , vst_path );
		if ( vst_path != NULL )
		{
			char *tok = strtok( tok_path , ";" );
			 while( tok != NULL )
			 {
				 CString abpath( tok );
				 if( abpath.Right( 1 ) != _T("\\") )
				 {
					abpath += "\\";
				 }				 
				 const char * realpath = findFilePath( abpath , dllname );				
				 //post( "findFilePath( %s , %s ) = %s\n" , abpath , dllname , realpath );
				 if ( realpath != NULL )
				 {
					 CString rpath( realpath );
					rpath += _T("\\") + str;
					 post( "trying %s " , rpath );
					if ( x->plug->Instance( rpath ) == VSTINSTANCE_NO_ERROR )
					{
						post( "  %s loaded " , x->plug->GetName());
						lf = TRUE;
						break;
					}
				 }
				tok = strtok( NULL , ";" );
				if ( tok == NULL )
				{
					post( "couldn't find dll");
				}
			 }
		}
	}
	if ( !lf ) // failed - don't make any ins or outs
	{
		post("Unable to load %s" , str );
		delete x->plug;
		x->plug = NULL;
		return( x);
	}

	// make our inlets and outlets next

	x->num_audio_inputs = x->plug->getNumInputs();
	x->num_audio_outputs = x->plug->getNumOutputs();

  /* Allocate memory for in- and outlet pointers */
    x->audio_inlets = (t_inlet**)calloc (x->num_audio_inputs, sizeof (t_inlet*));
    x->audio_outlets = (t_outlet**)calloc (x->num_audio_outputs, sizeof (t_outlet*));


    /* The first inlet is always there (needn't be created), and is
       used for control messages.  Now, create the rest of the
       inlets for audio signal input. */ 
    for (i = 0; i < x->num_audio_inputs; i++) 
	{
		x->audio_inlets[i] = inlet_new (&x->x_obj,&x->x_obj.ob_pd,gensym ("signal"),gensym ("signal"));
    }

	 // set up our inlets for midi notes
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("midinote"));	
	floatinlet_new(&x->x_obj, &x->x_vel);

    /* We use the first outlet always for VST parameter control  messages */
    x->control_outlet = outlet_new (&x->x_obj, gensym ("control"));

    /* The rest of the outlets are used for audio signal output */
    for (i = 0; i < x->num_audio_outputs; i++) 
	{
		x->audio_outlets[i] = outlet_new (&x->x_obj, gensym ("signal"));
    }
	
	return (x);
}

static const char * findFilePath( const char * path , const char * dllname )
{
	CFileFind finder;
	_chdir( path );
	if ( finder.FindFile( dllname ) == TRUE )
	{
		return path;
	}
	else
	{
		finder.FindFile();
		while( finder.FindNextFile() )
		{
			if ( finder.IsDirectory() )
			{
				if ( !finder.IsDots() )
				{
					CString *npath = new CString( finder.GetFilePath()); 
					const char * ret = findFilePath( *npath , dllname );
					if ( ret != NULL)
					{
						CString *retstr = new CString( ret );
						return *retstr;
					}
				}
			}
		}
	}
	return NULL;
}


/**
*	setup - add our methods 
*/

 void vst_tilde_setup(void)
{	
	 srand( (unsigned) time( NULL ) );
    vst_tilde_class = class_new(gensym("vst~"), (t_newmethod) vst_tilde_new, (t_method) vst_tilde_free,
    	sizeof(t_vst_tilde), 0, A_GIMME , 0);    
    CLASS_MAINSIGNALIN( vst_tilde_class, t_vst_tilde, x_f);
    class_addmethod(vst_tilde_class, (t_method) vst_tilde_dsp, gensym("dsp"), (t_atomtype)0);    	
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_control,gensym ("control"),A_DEFSYM, A_DEFFLOAT, 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_pitchbend,gensym ("pitchbend"),A_DEFFLOAT, 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_program,gensym ("program"),A_DEFFLOAT, 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_programchange,gensym ("programchange"),A_DEFFLOAT, 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_ctrlchange,gensym ("ctrlchange"),A_DEFFLOAT ,A_DEFFLOAT, 0);
    class_addmethod (vst_tilde_class,(t_method)vst_tilde_print,gensym ("print"),A_GIMME,(t_atomtype) 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_edit,gensym ("edit"),(t_atomtype) 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_showparams,gensym ("showparams"),(t_atomtype) 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_noshowparams,gensym ("noshowparams"),(t_atomtype) 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_param,gensym ("param"), A_DEFFLOAT , A_DEFFLOAT , (t_atomtype) 0);
    class_addmethod (vst_tilde_class,(t_method)vst_tilde_reset,gensym ("reset"),(t_atomtype) 0);
	class_addmethod (vst_tilde_class,(t_method)vst_tilde_midinote,gensym ("midinote"),A_DEFFLOAT,(t_atomtype) 0);	
	//class_sethelpsymbol(vst_tilde_class, gensym("vst/vsthelp"));
}

 static void vst_tilde_control (t_vst_tilde* x,  t_symbol* ctrl_name,t_float ctrl_value)     
{
    unsigned parm_num = 0;
    
    if (ctrl_name->s_name == NULL || strlen (ctrl_name->s_name) == 0) 
	{
		error ("plugin~: control messages must have a name and a value");
		return;
    }
    //parm_num = vst_tilde_get_parm_number (x, ctrl_name->s_name);
    //if (parm_num) 
	//{
		//vst_tilde_set_control_input_by_index (x, parm_num - 1, ctrl_value);
    //}
    //else 
	//{
		//vst_tilde_set_control_input_by_name (x, ctrl_name->s_name, ctrl_value);
    //}
}

 static void vst_tilde_pitchbend (t_vst_tilde* x,  t_float ctrl_value)     
{
	x->plug->AddPitchBend( (int) ctrl_value );    
}

  static void vst_tilde_programchange (t_vst_tilde* x,  t_float ctrl_value)     
{
	x->plug->AddProgramChange( (int) ctrl_value );    
}

static void vst_tilde_program (t_vst_tilde* x,  t_float ctrl_value)     
{
	x->plug->SetCurrentProgram( (int) ctrl_value );    
}

   static void vst_tilde_ctrlchange (t_vst_tilde* x, t_float control , t_float ctrl_value)     
{
	x->plug->AddControlChange( (int) control , (int) ctrl_value );    
}


 /**
 *	display the parameters names and values and some other bits and pieces that 
 *	may be of use
 */

static void vst_tilde_print (t_vst_tilde* x, t_symbol *s, int ac, t_atom *av ) 
{
     int i;
	bool params = false;
	bool header = true;
	bool programs = false;
	bool parameters = true;
	int specific = -1;
	 if ( ac > 0 )
	 {
		for( i = 0 ; i < ac ; i++)
		{
			if ( av[i].a_type == A_SYMBOL )
			{
				char buf[255];	
				atom_string( &av[i] , buf, 255 );
				if ( strcmp( buf , "-params" ) == 0 )
				{
					params = true;
				}
				else if ( strcmp( buf , "-noheader" ) == 0 )
				{
					header = false;
				}
				else if ( strcmp( buf , "-programs" ) == 0 )
				{
					programs = true;
					parameters = false;
				}
				else if ( strcmp( buf , "-parameters" ) == 0 )
				{				
					parameters = false;
				}
				else if ( strcmp( buf , "-help" ) == 0 )
				{
					post("print options:");
					post("-help \t\tprint this");
					post("-params \tshow the parameter display values ");
					post("-noheader \tdo not display the header");
					return;
				}
			}
			else if ( av[i].a_type == A_FLOAT )
			{
				int p = (int) atom_getfloat( &av[i] );
				if (( p > 0 ) && ( p <=  x->plug->GetNumParams()))
				{
					specific = p - 1;
				}
			}
		}
	 }
	 if ( header )
	 {
		post("VST~ plugin: %s " , x->plug->GetName() );
		post("made by: %s " , x->plug->GetVendorName() );
		post("parameterss %d\naudio: %d in(s)/%d out(s) \nLoaded from library \"%s\".\n",
			x->plug->GetNumParams(),
			x->num_audio_inputs,
			x->num_audio_outputs,
			x->plug->GetDllName());

		post("Flags");
		if ( x->plug->_pEffect->flags & effFlagsHasEditor )
		{
			post("Has editor");
		}
		if ( x->plug->_pEffect->flags & effFlagsCanReplacing )
		{
			post("Can do replacing");
		}
	 }
	 if ( parameters )
	 {
		if ( specific == -1)
		{
			for (i = 0; i < x->plug->GetNumParams(); i++) 
			{
				display_parameter( x , i , params );	
			}
		}
		else
		{
			display_parameter( x , specific , params);	
		}
	 }
	 if( programs )
	 {
		 
		for( int j = 0; j < x->plug->GetNumCategories() ; j++ )
		{
			for( i = 0 ; i < x->plug->GetNumParams() ; i++ )
			{
				char buf[64];
				x->plug->GetProgramName( j , i , buf );
				post("Program %d: %s ", i , buf );
			}
		}
	 }
}

static void display_parameter( t_vst_tilde* x,  int param , bool showparams )
{
	int j = param;
	/* the Steinberg(tm) way... */
	char name[109];
	char display[164];
	float val;
	if (j == 0) 
	{
		post ("Control input/output(s):");
	}
	memset (name, 0, 108);
	memset( display, 0 ,163);
	x->plug->GetParamName( j , name );
	if ( name[0] != NULL )
	{
		if ( showparams )
		{
			x->plug->DescribeValue( j , display );		
			val = x->plug->GetParamValue( j );
			post ("parameter[#%d], \"%s\" value=%f (%s) ", j + 1, name,  val,display);			
		}
		else
		{
			val = x->plug->GetParamValue( j );
			post ("parameter[#%d], \"%s\" value=%f ", j + 1, name,  val);			
		}
	}
}


/**
*	display an editor - currently not implemented 
*/


static void vst_tilde_edit(t_vst_tilde* x)     
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());	
	if ( x->plug != NULL )
	{
		x->plug->edit();    
	}
	else
	{
		post("No plugin to edit");
	}
}

static void vst_tilde_showparams(t_vst_tilde* x)     
{
	x->plug->SetShowParameters( true);    
}

static void vst_tilde_noshowparams(t_vst_tilde* x)     
{
	x->plug->SetShowParameters( false);    
}



/**
*	set the value of a parameter
*/

static void vst_tilde_param(t_vst_tilde* x , t_float pnum , t_float val)     
{
	if ( ( pnum > 0 ) && ( pnum <= x->plug->GetNumParams() ))
	{		
		int i = (int) pnum - 1;
		char name[9];
		char display[64];
		float xval;
		memset (name, 0, 9);
		memset( display, 0 ,64);
		x->plug->GetParamName( i , name );
		if ( name[0] != NULL )
		{		
			xval = x->plug->GetParamValue( i );
			if ( xval <= 1.0f)
			{
				x->plug->SetParameter( i , val );
				if ( x->plug->ShowParams() )
				{
					display_parameter( x , i , true );
				}
			}		
		}
	}
}

static void vst_tilde_reset (t_vst_tilde* x)
{
    
}

static void vst_tilde_midinote(t_vst_tilde* x , t_float note )     
{
	if ( x->x_vel > 0 )
	{
		x->plug->AddNoteOn( note , x->x_vel );
	}
	else
	{
		x->plug->AddNoteOff( note );
	}
}

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
}
#endif