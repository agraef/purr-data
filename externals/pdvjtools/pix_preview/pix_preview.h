
/*-----------------------------------------------------------------
pix_preview

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_DUMP_H_
#define INCLUDE_PIX_DUMP_H_

#include "Base/GemPixObj.h"

/*needed for base64 conversion*/
#include <string>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

std::string base64_encode(unsigned char const* , unsigned int len);


/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS

 pix_preview
 
  Get pixel information
  
   KEYWORDS
   pix
   
	DESCRIPTION
	
	 dumps the pix-data as a float-package
	 
-----------------------------------------------------------------*/
class GEM_EXTERN pix_preview : public GemPixObj
{
	CPPEXTERN_HEADER(pix_preview, GemPixObj)
		
public:
	
	//////////
	// Constructor
	pix_preview(t_floatarg fx, t_floatarg fy);
	int x_width, x_height;
	static int counter;
	
protected:
	
	//////////
	// Destructor
	virtual ~pix_preview();
	
	//////////
	// All we want is the pixel information, so this is a complete override.
	virtual void 	processImage(imageStruct &image);
	
	//////////
	virtual void 	processYUVImage(imageStruct &image);
	
	//////////
	void			trigger();
	int getx();
	int gety();
    	void	    	connectMess();

	//////////
	// The color outlet
	t_outlet    	*m_dataOut;
	
	//////////
	// the buffer
	int           xsize, ysize;      // proposed x/y-sizes
	int           m_xsize,  m_ysize;
	int           m_csize;
	t_atom       *m_buffer;
	int           m_bufsize;
	
	int           oldimagex;
	int           oldimagey;
	int           widgetwidth;
	int           widgetheight;
	
	//////////
	// navigation
	float         m_xstep;
	float         m_ystep;
	
	/////////
	// pointer to the image data
	unsigned char *m_data;
	
	/////////
	// LLUO :: widget tk list pointer
    	t_glist *m_glist;
	
	t_widgetbehavior   image_widgetbehavior;

	int s;
	int connected;
	struct addrinfo *res;
	struct addrinfo hints;
	int 		whoami;


private:
	
	//////////
	// Static member callbacks
	static void		triggerMessCallback(void *dump);
	static void		GREYMessCallback(void *dump);
	static void		RGBAMessCallback(void *dump);
	static void		RGBMessCallback(void *dump);
    	static void 		connectMessCallback(void *dump);


	static void image_drawme(pix_preview *x, t_glist *glist, int firsttime, int m_xsize, int m_ysize);

	static void image_erase(pix_preview* x,t_glist* glist);
	

/* ------------------------ image widgetbehaviour----------------------------- */


	static void image_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2);

	static void image_displace(t_gobj *z, t_glist *glist, int dx, int dy);

	static void image_select(t_gobj *z, t_glist *glist, int state);

	static void image_activate(t_gobj *z, t_glist *glist, int state);

	static void image_delete(t_gobj *z, t_glist *glist);
       
	static void image_vis(t_gobj *z, t_glist *glist, int vis);

};

#endif	// for header file
