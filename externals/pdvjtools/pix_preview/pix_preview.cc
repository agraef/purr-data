
/////////////////////////////////////////////////////////
//
//  pix_preview
//
//  Lluis Gomez i Bigorda
//  mailto:lluis@artefacte.org
//
/////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <sstream>
using namespace std;
#include "stdio.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"


#include "pix_preview.h"

#define initialport 1234

int guidebug=1;

char *fdata="R0lGODlhHAAcAIABAAAAAP///ywAAAAAHAAcAAACGoSPqcvtD6OctNqLs968+w+G4kiW5omm6ooUADs=";

int pix_preview::counter;

/* base64 conversion*/
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}


CPPEXTERN_NEW_WITH_TWO_ARGS(pix_preview, t_floatarg, A_DEFFLOAT, t_floatarg, A_DEFFLOAT)


  /////////////////////////////////////////////////////////
  //
  // pix_preview
  //
  /////////////////////////////////////////////////////////
  // Constructor
  //
  /////////////////////////////////////////////////////////
  pix_preview :: pix_preview(t_floatarg fx, t_floatarg fy)
{ 
    counter++;
    whoami = counter;
  #include "pix_preview.tk2c"
  sys_vgui("Echo_Server%d %d\n",whoami, 1233+whoami);
  xsize = (int)fx;
  ysize = (int)fy;
  m_csize = 3;


    image_widgetbehavior.w_getrectfn =     image_getrect;
    image_widgetbehavior.w_displacefn =    image_displace;
    image_widgetbehavior.w_selectfn =   image_select;
    image_widgetbehavior.w_activatefn =   image_activate;
    image_widgetbehavior.w_deletefn =   image_delete;
    image_widgetbehavior.w_visfn =   image_vis;
#if (PD_VERSION_MINOR > 31) 
    image_widgetbehavior.w_clickfn = NULL;
    image_widgetbehavior.w_propertiesfn = NULL; 
#endif
#if PD_MINOR_VERSION < 37
    image_widgetbehavior.w_savefn =   image_save;
#endif
    

  class_setwidget(pix_preview_class,&image_widgetbehavior);

  if (xsize < 0) xsize = 0;
  if (ysize < 0) ysize = 0;

  m_xsize = xsize;
  m_ysize = ysize;

  oldimagex = xsize;
  oldimagey = ysize;

  m_bufsize = m_xsize * m_ysize * m_csize;

  m_buffer = new t_atom[m_bufsize];

    widgetwidth = 0;
    widgetheight = 0;

  //m_dataOut = outlet_new(this->x_obj, &s_list);

  // Try to connect (not working now , using connectMes)
  memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::ostringstream ostr;
    ostr << 1233+whoami;
    std::string str;
    str = ostr.str();
    
    getaddrinfo("localhost", reinterpret_cast<const char*>(str.c_str()), &hints, &res); 

	fprintf(stderr,"trying to connect...\n");
  	s = socket(res->ai_family, res->ai_socktype, 0);
  	connect(s, res->ai_addr, res->ai_addrlen);

    //fprintf(stderr,"We have %d pixpreviews\n",counter);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_preview :: ~pix_preview()
{
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_preview :: connectMess() 
{
  memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::ostringstream ostr;
    ostr << 1233+whoami;
    std::string str;
    str = ostr.str();
    
    getaddrinfo("localhost", reinterpret_cast<const char*>(str.c_str()), &hints, &res); 

	fprintf(stderr,"trying to connect...\n");
  	s = socket(res->ai_family, res->ai_socktype, 0);
  	connect(s, res->ai_addr, res->ai_addrlen);
}

void pix_preview :: processImage(imageStruct &image)
{
  int x = m_xsize, y = m_ysize, c = m_csize;

  if (image.xsize != oldimagex) {
    oldimagex = image.xsize;
    m_xsize = ((!xsize) || (xsize > oldimagex))?oldimagex:xsize;
  }
  if (image.ysize != oldimagey) {
    oldimagey = image.ysize;
    m_ysize = ((!ysize) || (ysize > oldimagey))?oldimagey:ysize;
  }

  if (image.csize != m_csize) m_csize = image.csize;

  if ( (m_xsize != x) || (m_ysize != y) || (m_csize != c) ) {
    // resize the image buffer
    if(m_buffer)delete [] m_buffer;
    m_bufsize = m_xsize * m_ysize * m_csize;
    m_buffer = new t_atom[m_bufsize];

    m_xstep = m_csize * ((float)image.xsize/(float)m_xsize);
    m_ystep = m_csize * ((float)image.ysize/(float)m_ysize) * image.xsize;
  }

  m_data = image.data;
}

/////////////////////////////////////////////////////////
// processYUVImage
//
/////////////////////////////////////////////////////////
void pix_preview :: processYUVImage(imageStruct &image)
{
    int x = m_xsize, y = m_ysize, c = m_csize;

  if (image.xsize != oldimagex) {
    oldimagex = image.xsize;
    m_xsize = ((!xsize) || (xsize > oldimagex))?oldimagex:xsize;
  }
  if (image.ysize != oldimagey) {
    oldimagey = image.ysize;
    m_ysize = ((!ysize) || (ysize > oldimagey))?oldimagey:ysize;
  }

  if (image.csize != m_csize) m_csize = image.csize;

  if ( (m_xsize != x) || (m_ysize != y) || (m_csize != c) ) {
    // resize the image buffer
    if(m_buffer)delete [] m_buffer;
    m_bufsize = m_xsize * m_ysize * m_csize;
    m_buffer = new t_atom[m_bufsize];

    m_xstep = m_csize * ((float)image.xsize/(float)m_xsize);
    m_ystep = m_csize * ((float)image.ysize/(float)m_ysize) * image.xsize;
  }

  m_data = image.data;
}

/////////////////////////////////////////////////////////
// trigger
//
/////////////////////////////////////////////////////////
void pix_preview :: trigger()
{
  if (!m_data) return;

  
  int n = m_ysize, m = 0;
  int i = 0;

  unsigned char *data, *line;
  stringstream sx,sy;

	//fprintf (stderr,"%d %d %d %d %d %d \n",xsize, ysize,m_xsize,  m_ysize,oldimagex,oldimagey);

	std::string pnm;
	std::string pnm64;
	pnm += "P6\n";
		sx << m_xsize;
	pnm += sx.str();
	pnm += " ";
		sy << m_ysize;
	pnm += sy.str();
	pnm += "\n255\n";

  data = line = m_data;
  switch(m_csize){
  case 4:
    while (n > 0) {
      while (m < m_xsize) {
	int r, g, b, a;
	r = (int)data[chRed];
                pnm += (char)r;
	i++;
	g = (int)data[chGreen];
                pnm += (char)g;
	i++;
	b = (int)data[chBlue];
                pnm += (char)b;
	i++;
	a = (int)data[chAlpha];
	i++;
	m++;
	data = line + (int)(m_xstep * (float)m);
      }
      m = 0;
      n--;
      line = m_data + (int)(m_ystep*n);
      data = line;
    }

	
	//std::cout << "NOT encoded: " << pnm << std::endl;

	pnm64 = base64_encode(reinterpret_cast<const unsigned char*>(pnm.c_str()), pnm.length());
	//std::cout << "encoded: " << pnm64 << std::endl;
	
	pnm64 += "\n";

	send(s, reinterpret_cast<const unsigned char*>(pnm64.c_str()), pnm64.length(), 0);
		
	//m_glist = (t_glist *) canvas_getcurrent();

	sys_vgui(".x%x.c coords %xS %d %d\n",
		   this->getCanvas(), this->x_obj,
		   text_xpix(this->x_obj, (t_glist *)this->getCanvas()) + (m_xsize/2), text_ypix(this->x_obj, (t_glist *)this->getCanvas()) + (m_ysize/2));
	
    break;
  case 2:
    while (n < m_ysize) {
      while (m < m_xsize/2) {
	float y,u,y1,v;
	u = (float)data[0] / 255.f;
	SETFLOAT(&m_buffer[i], u);
	i++;
	y = (float)data[1] / 255.f;
	SETFLOAT(&m_buffer[i], y);
	i++;
	v = (float)data[2] / 255.f;
	SETFLOAT(&m_buffer[i], v);
	i++;
	y1 = (float)data[3] / 255.f;
	SETFLOAT(&m_buffer[i], y1);
	i++;
	m++;
	data = line + (int)(m_xstep * (float)m);
      }
      m = 0;
      n++;
      line = m_data + (int)(m_ystep*n);
      data = line;
    }
  case 1:  default:
    int datasize=m_xsize*m_ysize*m_csize/4;
      while (datasize--) {
	float v;
	v = (float)(*data++) / 255.f;	  SETFLOAT(&m_buffer[i], v);
	v = (float)(*data++) / 255.f;	  SETFLOAT(&m_buffer[i+1], v);
	v = (float)(*data++) / 255.f;	  SETFLOAT(&m_buffer[i+2], v);
	v = (float)(*data++) / 255.f;	  SETFLOAT(&m_buffer[i+3], v);
	i+=4;
      }
  }
  //outlet_list(m_dataOut, gensym("list"), i, m_buffer);
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_preview :: obj_setupCallback(t_class *classPtr)
{
  class_addbang(classPtr, (t_method)&pix_preview::triggerMessCallback);
  class_addmethod(classPtr, (t_method)&pix_preview::connectMessCallback, gensym("connect"), A_NULL);
}

void pix_preview :: triggerMessCallback(void *data)
{
  GetMyClass(data)->trigger();
}

void pix_preview :: connectMessCallback(void *data)
{
  GetMyClass(data)->connectMess();
}




/* widget helper functions */
void pix_preview :: image_drawme(pix_preview *x, t_glist *glist, int firsttime, int m_xsize, int m_ysize)
{
       if (firsttime) {
	  sys_vgui("image create photo imgPREVIEW%d -data {%s}\n",x->counter,fdata);
	  fprintf(stderr,"image create photo imgPREVIEW%d -data {%s}\n",x->counter,fdata);
	  sys_vgui(".x%x.c create image %d %d -image imgPREVIEW%d -tags %xS\n", glist_getcanvas(glist),text_xpix((t_object*)x, glist)+14, text_ypix((t_object*)x, glist)+14,x->counter,x);
     }     
}

void pix_preview :: image_erase(pix_preview* x,t_glist* glist)
{
     int n;
     sys_vgui(".x%x.c delete %xS\n",
	      glist_getcanvas(glist), x);
}
	

/* ------------------------ image widgetbehaviour----------------------------- */


void pix_preview :: image_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{    
    int width, height;
    pix_preview *x = (pix_preview *)z;


    *xp1 = text_xpix((t_object*)x, glist);
    *yp1 = text_ypix((t_object*)x, glist);
    *xp2 = text_xpix((t_object*)x, glist) + x->widgetwidth;
    *yp2 = text_ypix((t_object*)x, glist) + x->widgetheight;
}

void pix_preview :: image_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_object *x = (t_object *)z;
    x->te_xpix += dx;
    x->te_ypix += dy;
    sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
		   glist_getcanvas(glist), x,
		   text_xpix(x, glist), text_ypix(x, glist),
		   text_xpix(x, glist) + 25, text_ypix(x, glist) + 25);

    //image_drawme((pix_preview *)x, glist, 0);
    canvas_fixlinesfor(glist_getcanvas(glist),(t_text*) x);
}

void pix_preview :: image_select(t_gobj *z, t_glist *glist, int state)
 {   
     t_object *x = (t_object *)z;
     pix_preview *s = (pix_preview *)z;
     if (state) {
	  sys_vgui(".x%x.c create rectangle \
%d %d %d %d -tags %xSEL -outline blue\n",
		   glist_getcanvas(glist),
		   text_xpix(x, glist), text_ypix(x, glist),
		   text_xpix(x, glist) + 25, text_ypix(x, glist) + 25,
		   x);
    	s->widgetwidth = 28;
    	s->widgetheight = 28;
     }
     else {
	  sys_vgui(".x%x.c delete %xSEL\n",
		   glist_getcanvas(glist), x);
    	s->widgetwidth = 0;
    	s->widgetheight = 0;
     }
}


void pix_preview :: image_activate(t_gobj *z, t_glist *glist, int state)
{
}

void pix_preview :: image_delete(t_gobj *z, t_glist *glist)
{

    t_text *x = (t_text *)z;
    pix_preview* s = (pix_preview*)z;
    canvas_deletelinesfor(glist_getcanvas(glist), x);
    image_erase(s, glist);
}

       
void pix_preview :: image_vis(t_gobj *z, t_glist *glist, int vis)
{
    pix_preview* s = (pix_preview*)z;
    if (vis)
	 image_drawme(s, glist, 1, 28, 28);
  
}

