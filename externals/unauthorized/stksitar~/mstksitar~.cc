/* stksitar~ -- apply a stksitar effect 
 * requires libDSP library
 * Copyleft 2001 Yves Degoyon.
 * Permission is granted to use this software for any purpose provided you
 * keep this copyright notice intact.
 *
 * THE AUTHOR AND HIS EXPLOITERS MAKE NO WARRANTY, EXPRESS OR IMPLIED,
 * IN CONNECTION WITH THIS SOFTWARE.
 *
*/

#include "sitar.h"
#include "unistd.h"
#include "RtWvOut.h"

sitar *x_stksitar = NULL;

int main( int argc, char** argv )
{

  // int count=0;
    
    x_stksitar = new sitar( 50.0 );
    if ( x_stksitar == NULL ) 
    {
       printf( "mstksitar~: cannot build sitar instrument from STK" );
       exit(-1);
    }

    x_stksitar->noteOn( 400.0, 0.25 ); // start sound
    while (1)
    {
      double dare;
        
       dare = (float) x_stksitar->tick();
       printf( "%f\n", dare );
       fwrite( (void*)&dare, sizeof(float), 1, stderr );
    }
}
