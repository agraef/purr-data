
extern "C"
{
    /*
    videogrid external for Puredata
    Lluis Gomez i Bigorda :: lluis-at-hangar.org
    Sergi Lario Loyo      :: slario-at-gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

    #include <stdio.h>
    #include <stdlib.h>
    #include <dirent.h>
    #include <string.h>
    #include <unistd.h>
    #include <math.h>

    /*
     * incloure les definicions de variables i prototipus de dades i de funcions de puredata
    */
    #include "m_pd.h"
    /*
     * incloure estructures de dades i capceleres de funcions gàfiques bàsiques de pd
    */
    #include "g_canvas.h"
    #include "s_stuff.h"
    /*
     * incloure estructures de dades i capceleres de funcions per traballar amb threads
    */
    #include "pthread.h"

    /* some linux distros need this to compile ffmpeg correctly as cpp */
    #define __STDC_CONSTANT_MACROS 1

    /* ffmpeg includes */

    #include <ffmpeg/avcodec.h>
    #include <ffmpeg/avformat.h>
    #include <ffmpeg/avutil.h>
    #include <ffmpeg/swscale.h>


    /* may be your ffmpeg headers are here, at least from jaunty to karmic */
    /*
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libswscale/swscale.h>
    */

    /* libquicktime includes */
    // #include <quicktime/lqt.h>
    // #include <quicktime/colormodels.h>
    #include <lqt/lqt.h>
    #include <lqt/colormodels.h>

    /* &&&&&&&&&&&&&&&&&&&&&&&&&&&&& VIDEOGRID &&&&&&&&&&&&&&&&&&&&&&&&&&&&& */

    /* definició del gruix en pixels del marc del tauler */
    #define GRUIX 2

    /* nombre de caracters per el nom del path del fitxer */
    #define BYTESNOMFITXER 512

    /* 8bits clamp rgb values */
    #define CLAMP8(x) (((x)<0) ? 0 : ((x>255)? 255 : (x)))

    #define BYTESNOMFITXERIMATGE 512
    #define BYTESTIPUSFROMAT 4

    #define FORMAT_MINIATURA "ppm"
    #define PATH_TEMPORAL "/tmp/vigrid_"
    #define BYTES_NUM_TEMP 4

    typedef char pathimage[BYTESNOMFITXERIMATGE];

    typedef char tipus_format[BYTESTIPUSFROMAT];

    t_namelist *loaded_libs = NULL;


    /* ----------------------------------- FFmpeg functions ----------------------------------- */
    int convertir_img_ff(pathimage pathFitxer, tipus_format f, int W, int H, int posi);

    void SaveFrame(AVFrame *pFrame, int width, int height, int W, int H, int posi)
    {
        FILE *pFile;
        char szFilename[32];
        int nN = posi;

        char nNstr[BYTES_NUM_TEMP];
        pathimage  ig_path = PATH_TEMPORAL;

        sprintf(nNstr, "%d", nN);
        strcat(ig_path,nNstr);
        strcat(ig_path,".");
        strcat(ig_path,FORMAT_MINIATURA);

        // Open file
        sprintf(szFilename, ig_path);
        pFile=fopen(szFilename, "wb");
        if(pFile==NULL)
            return;

        // Write header
        fprintf(pFile, "P6\n%d %d\n255\n", W, H);

        int w = width;
        int h = height;
        float k = (width/W);
        float l = (height/H);
        int i,j,y,x,realx;

        // Write pixel data
        for(y=0; y<H; y=y++) {
            for(x=0; x<W; x=x++) {
                realx = ((x*k)+(3-((int)(x*k)%3)))*3;
                fwrite((pFrame->data[0]+(int)(y*l)*pFrame->linesize[0])+realx, 1, 3, pFile);
            }
        }

        // Close file
        fclose(pFile);
    }

    int convertir_img_ff(pathimage pathFitxer, tipus_format f, int W, int H, int posi)
    {
        AVFormatContext *pFormatCtx;
        int             i, videoStream;
        AVCodecContext  *pCodecCtx;
        AVCodec         *pCodec;
        AVFrame         *pFrame;
        AVFrame         *pFrameRGB;
        AVPacket        packet;
        int             frameFinished;
        int             numBytes;
        uint8_t         *buffer;
        static int sws_flags = SWS_BICUBIC;
        struct SwsContext *img_convert_ctx;

        int nN = posi;

        // Register all formats and codecs
        av_register_all();

        // Open video file
        if(av_open_input_file(&pFormatCtx, pathFitxer, NULL, 0, NULL)!=0)
            return -1; // Couldn't open file

        // Retrieve stream information
        if(av_find_stream_info(pFormatCtx)<0)
            return -1;  // Couldn't find stream information

        // Dump information about file onto standard error
        dump_format(pFormatCtx, 0, pathFitxer, false);

        // Find the first video stream
        videoStream=-1;
        for(i=0; i<pFormatCtx->nb_streams; i++)
            if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO)
            {
                videoStream=i;
                break;
            }
        if(videoStream==-1)
            return -1; // Didn't find a video stream

        // Get a pointer to the codec context for the video stream
        pCodecCtx=pFormatCtx->streams[videoStream]->codec;

        // Find the decoder for the video stream
        pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
        if(pCodec==NULL)
            return -1; // Codec not found

        // Open codec
        if(avcodec_open(pCodecCtx, pCodec)<0)
            return -1; // Could not open codec

        // Hack to correct wrong frame rates that seem to be generated by some
        // codecs
        // if(pCodecCtx->frame_rate>1000 && pCodecCtx->frame_rate_base==1)
        //    pCodecCtx->frame_rate_base=1000;

        // Allocate video frame
        pFrame=avcodec_alloc_frame();

        // Allocate an AVFrame structure
        pFrameRGB=avcodec_alloc_frame();
        if(pFrameRGB==NULL)
            return -1;

        // Determine required buffer size and allocate buffer
        numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
            pCodecCtx->height);
        buffer=new uint8_t[numBytes];

        // Assign appropriate parts of buffer to image planes in pFrameRGB
        avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
            pCodecCtx->width, pCodecCtx->height);

        // Read frames and save first five frames to disk
        av_read_frame(pFormatCtx, &packet);

        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream)
        {
            // Decode video frame
            avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, packet.data, packet.size);

            // Did we get a video frame?
            if(frameFinished)
            {
                // Convert the image from its native format to RGB
                //img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,
                //    (AVPicture*)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width,
                //    pCodecCtx->height);
                img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                    pCodecCtx->pix_fmt,
                                    pCodecCtx->width, pCodecCtx->height,
                                    PIX_FMT_RGB24,
                                    sws_flags, NULL, NULL, NULL);

                sws_scale (img_convert_ctx, pFrame->data, pFrame->linesize,
                            0, pCodecCtx->height,
                            pFrameRGB->data, pFrameRGB->linesize);
                sws_freeContext(img_convert_ctx);

                // Save the frame to disk
                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, W, H, posi);
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
        // Read frames and save first five frames to disk
        av_read_frame(pFormatCtx, &packet);
        // Is this a packet from the video stream?
        if(packet.stream_index==videoStream)
        {
            // Decode video frame
            avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,
                packet.data, packet.size);

            // Did we get a video frame?
            if(frameFinished)
            {
                // Convert the image from its native format to RGB
                //img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24,
                //    (AVPicture*)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width,
                //    pCodecCtx->height);
                img_convert_ctx = sws_getContext(    pCodecCtx->width, pCodecCtx->height,
                                    pCodecCtx->pix_fmt,
                                    pCodecCtx->width, pCodecCtx->height,
                                    PIX_FMT_RGB24,
                                    sws_flags, NULL, NULL, NULL);

                sws_scale (img_convert_ctx, pFrame->data, pFrame->linesize,
                            0, pCodecCtx->height,
                            pFrameRGB->data, pFrameRGB->linesize);
                sws_freeContext(img_convert_ctx);

                // Save the frame to disk
                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, W, H, posi);
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);

        // Free the RGB image
        delete [] buffer;
        av_free(pFrameRGB);

        // Free the YUV frame
        av_free(pFrame);

        // Close the codec
        avcodec_close(pCodecCtx);

        // Close the video file
        av_close_input_file(pFormatCtx);

        return 0;
    }

    /* ----------------------------------- Quicktime functions ----------------------------------- */
    int convertir_img(pathimage pathFitxer, tipus_format f, int W, int H, int posi)
    {
        /*
        Quicktime per les conversions
        */

        /* RGB vectors */
        unsigned char * qt_rows[3];
        /* YUV vesctor frame */
        unsigned char *qt_frame = NULL;
        /* quicktime decoder */
        quicktime_t *qt;
        /* quicktime color model */
        int qt_cmodel;

        int nN = posi;
        int x_vwidth = 0;
        int x_vheight = 0;
        qt = quicktime_open(pathFitxer, 1, 0);

        if (!(qt)){
            /* post("videogrid: error opening qt file"); */
            return -1;
        }

        if (!quicktime_has_video(qt)) {
            /* post("videogrid: no video stream"); */
            quicktime_close(qt);
            return -1;

        }
        else if (!quicktime_supported_video(qt,0)) {
            /* post("videogrid: unsupported video codec\n"); */
            quicktime_close(qt);
            return -1;
        }
        else
        {
            qt_cmodel = BC_YUV420P;
            x_vwidth  = quicktime_video_width(qt,0);
            x_vheight = quicktime_video_height(qt,0);

            free(qt_frame);
            qt_frame = (unsigned char*)malloc(x_vwidth*x_vheight*4);

            int size = x_vwidth * x_vheight;
            qt_rows[0] = &qt_frame[0];
            qt_rows[2] = &qt_frame[size];
            qt_rows[1] = &qt_frame[size + (size>>2)];

            quicktime_set_cmodel(qt, qt_cmodel);
        }

        /* int length = quicktime_video_length(qt,0); */
        /* int Vpos = quicktime_video_position(qt,0); */
        lqt_decode_video(qt, qt_rows, 0);

        switch(qt_cmodel){
            case BC_YUV420P:
                /* printf(" "); */
                /* post("videogrid: qt colormodel : BC_YUV420P"); */

                /* per a fer la miniatura
                   cada k colomnes pillem una
                   cada l files pillem una */
                int w = x_vwidth;
                int h = x_vheight;
                int k = (w/W);
                int l = (h/H);

                char nNstr[BYTES_NUM_TEMP];
                pathimage  ig_path = PATH_TEMPORAL;

                sprintf(nNstr, "%d", nN);
                strcat(ig_path,nNstr);
                strcat(ig_path,".");
                strcat(ig_path,FORMAT_MINIATURA);
                /* printf("Creacio de la imatge %s ...",ig_path); */
                /* escriu el contingut de data a un arxiu. */
                FILE *fp = fopen(ig_path, "w");
                fprintf (fp, "P6\n%d %d\n255\n", W, H);

                int i,j,y,u,v,r,g,b;

                for (i=0;i<(l*H);i=i+l) {
                    for (j=0;j<(k*W);j=j+k) {
                        y=qt_rows[0][(w*i+j)];
                        u=qt_rows[1][(w/2)*(i/2)+(j/2)];
                        v=qt_rows[2][(w/2)*(i/2)+(j/2)];
                        r = CLAMP8(y + 1.402 *(v-128));
                        g = CLAMP8(y - 0.34414 *(u-128) - 0.71414 *(v-128));
                        b = CLAMP8(y + 1.772 *(u-128));
                        fprintf (fp, "%c%c%c", r,g,b);
                    }
                }
                /* escriu el contingut de data a un arxiu.*/
                fclose (fp);
        }
        return 0;
    }

    /* ----------------------------------- Cue > rounded list ----------------------------------- */
    typedef char path[BYTESNOMFITXER];

    /* estructura de dades: un node de la cua */
    struct node
    {
        /* nom del path de la imatge */
        path pathFitxer;
        /* apuntador al següent node en cua */
        struct node *seguent;
    };

    /* definició del tipus node */
    typedef struct node Node;

    /* estructures i tipus de dades de la cua */
    /* definició del tipus de cua */
    typedef struct
    {
        Node *davanter;
        Node *final;
    }Cua;

    /* declaracions de les funcions */

    /* crea una cua */
    void crearCua(Cua *cua);
    /* encuara un element al final de la cua */
    void encuar (Cua *cua, path x);
    /* elimina un element de la cua */
    int desencuar (Cua *cua);
    /* retorna si la cua és buida */
    int cuaBuida(Cua *cua);
    /* elimina el contingut de la cua */
    void eliminarCua(Cua *cua);
    /* retorna el nombre de nodes de la cua */
    int numNodes(Cua *cua);
    /* escriu el contingut de la cua */
    void escriuCua(Cua *cua);

    /* implementació de les funcions */
    void crearCua(Cua *cua)
    {
        cua->davanter=cua->final=NULL;
    }

    /* funció que encua el node al final de la cua */
    void encuar (Cua *cua, path x)
    {
        Node *nou;
        nou=(Node*)malloc(sizeof(Node));
        strcpy(nou->pathFitxer,x);
        nou->seguent=NULL;
        if(cuaBuida(cua))
        {
            cua->davanter=nou;
        }
        else
            cua->final->seguent=nou;
        cua->final=nou;
    }

    /* elimina l'element del principi de la cua */
    int desencuar (Cua *cua)
    {
        if(!cuaBuida(cua))
        {
            Node *nou;
            nou=cua->davanter;
            cua->davanter=cua->davanter->seguent;
            free(nou);
            return 1;
        }
        else
        {
            /* printf("Cua buida\a\n"); */
            return 0;
        }
    }

    /* funció que retorna si la cua és buida */
    int cuaBuida(Cua *cua)
    {
        return (cua->davanter==NULL);
    }

    /* elimina el contingut de la cua */
    void eliminarCua(Cua *cua)
    {
        while (!cuaBuida(cua)) desencuar(cua);
        /* printf("Cua eliminada\n"); */
    }

    /* funció que retorna el nombre de nodes de la cua */
    int numNodes(Cua *cua)
    {
        int contador=0;
        Node *actual;
        actual=cua->davanter;
        if(actual) contador=1;
        while((actual)&&(actual != cua->final)){
             contador ++;
             actual = actual->seguent;
        }
        return (contador);
    }

    /* funció que escriu la cua de nodes */
    void escriuCua(Cua *cua)
    {
        if(!cuaBuida(cua))
        {
            Node *actual;
            actual=cua->davanter;
            post("THUMBS INSIDE\n[");
            do{
                post("#%s#",actual->pathFitxer);
                actual = actual->seguent;
            }while(actual);
            post("]\n");
        }
        else
            post("EMPTY: NO THUMBS INSIDE\n");
    }

    /* ----------------------------------- videogrid gui-videogrid ----------------------------------- */

    t_widgetbehavior   videogrid_widgetbehavior;
    /* crear un apuntador al nou objecte */
    static t_class *videogrid_class;
    /* indica el nombre de videogrid creats - utilitzat per diferenciar el nom d'instàncies d'objectes del mateix tipus */
    static int videogridcount = 0;

    /* definició de la classe i la seva estructura de dades */

    typedef struct _videogrid {
        t_object  x_obj;
        /* declaració de la sortida de l'objecte */
        t_outlet *x_sortida;
        /* llista d'objectes gràfics */
        t_glist *x_glist;
        /* nombre de files */
        int x_num_fil;
        /* nombre de columnes */
        int x_num_col;
        /* width del thumbnail  */
        int x_w_cell;
        /* height del thumbnail */
        int x_h_cell;
        /* posició de la última imatge en el tauler */
        int x_ultima_img;
        /* path del directori actual */
        path x_dir_actual;
        /* path del directori a canviar */
        path x_dir_canvi;
        /* posicio ultim al directori actual */
        int x_dir_pos;
        /* apuntador al primer element posicionat al tauler */
        Node *x_tauler_primer;
        /* cua d'imatges */
        Cua x_cua;
        /* nom de l'objecte */
        t_symbol *x_name;
        /* color de fons */
        t_symbol *x_color_fons;
        /* color del marge */
        t_symbol *x_color_marc;
        /* mutex per evitar concurrencia sobre la cua al accedir diferents threads*/
        pthread_mutex_t x_lock;
        /* v 0.2 -- posicó de la cel·la seleccionada */
        int x_pos_selected;
        /* v 0.2 -- color de seleccio */
        t_symbol *x_color_grasp;
        /* v 0.2.1 -- llista de formats */
        t_symbol *x_format_list;
    } t_videogrid;

    /* ---------------- control functions ------------ */

    /* v 0.2 -- mètode de la classe que desmarca la cel·la seleccionada */
    /* calcula la posició x del tauler a partir de la posició de l'element de la cua (d'esquerra a dreta) */
    int getX(t_videogrid* x, int posCua){
        int c = x->x_num_col;
        int xpos = (posCua % c) * x->x_w_cell + ((posCua % c) + 1) * GRUIX;
        return(xpos + 1);
    }

    /* calcula la posició y del tauler a partir de la posició de l'element de la cua (de dalt a baix) */
    int getY(t_videogrid* x, int posCua){
        int c = x->x_num_col;
        int ypos = (posCua / c) * x->x_h_cell + ((posCua / c) + 1) * GRUIX;
        return(ypos + 1);
    }

    static void videogrid_ungrasp_selected(t_videogrid *x)
    {
        /* post("Ungrasp selected thumb %d", x->x_pos_selected); */
        if(x->x_pos_selected > -1) {
            sys_vgui(".x%x.c delete %xGRASP\n", glist_getcanvas(x->x_glist), x);
            x->x_pos_selected = -1;
        }
    }

    /* elimina les imatges temporals */
    void eliminar_imatges_temporals(int maxim){
        FILE *fitxer;
        path path_total;
        int contador = 0;
        char contador_str[BYTES_NUM_TEMP];
        while(contador < maxim){
            strcpy(path_total,PATH_TEMPORAL);
            sprintf(contador_str,"%d", contador);
            strcat(path_total,contador_str);
            strcat(path_total,".");
            strcat(path_total,FORMAT_MINIATURA);
            /* elimina el fitxer si no hi ha cap problema */
            if(unlink(path_total)){
                /* post("Imatge temporal %s eliminada\n",path_total); */
            }
            contador++;
        }
        /* post("Videogrid: Imatges temporals eliminades\n",path_total); */
    }

    int format_adequat_v(path nomF, t_symbol *format_list){
        int retorn = 0;
        path  ig_path = "";
        strcat(ig_path,nomF);
        path fl;
        strcpy(fl,format_list->s_name);
        char *t1;
        char *ext;
        path extensio = "";
        for ( t1 = strtok(ig_path,".");
              t1 != NULL;
              t1 = strtok(NULL,".") )
            strcpy(extensio,t1);

        for ( ext = strtok(fl,":");
          ext != NULL;
              ext = strtok(NULL,":") ) {
            if(strcmp(extensio,ext)==0) {
                retorn = 1;
                ext = NULL;
            }
        }
        return (retorn);
    }

    /* afegir una imatge al grid */
    void videogrid_afegir_imatge(t_videogrid *x, path entrada)
    {
        int maxim;
        char nNstr[BYTES_NUM_TEMP];
        int pos = 0;
        maxim = x->x_num_fil * x->x_num_col;
        path  ig_path = PATH_TEMPORAL;
        /* si hi ha tants nodes a la cua com el maxim */
        if((numNodes(&x->x_cua)) >=  maxim){
            /* desencua */
            int extret;
            extret = desencuar(&x->x_cua);
            /* obtenir la posició en la cua del nou node */
            if(x->x_ultima_img == maxim - 1) {
                pos = 0;
            }else{
                pos = x->x_ultima_img + 1;
            }
            sys_vgui(".x%x.c delete %xS%d\n", glist_getcanvas(x->x_glist), x, x->x_ultima_img);
        }

        /* FFMPEG o Quicktime per les conversions */
        int fferror=0;
        int nN = x->x_ultima_img;
        /*
        if (format_adequat_v(entrada, x->x_format_list) == 0) {
            // convertir_img(entrada,FORMAT_MINIATURA, x->x_w_cell, x->x_h_cell, nN);
            fferror = -1;

        } else {
        */
        fferror=convertir_img_ff(entrada,FORMAT_MINIATURA, x->x_w_cell, x->x_h_cell, nN);
        /* } */
        /* post ("%d",fferror); */
        if (fferror>=0) {
            /* encua el nou node */
            encuar(&x->x_cua, entrada);
            /* si no és el primer element a encuar incrementem la posicio de la última imatge insertada */
            if(numNodes(&x->x_cua) != 0) x->x_ultima_img ++;
            /*
             * si assoleix el maxim torna a començar,
             * inicialitzant la posició en el tauler de la última imatge insertada
             */
            if(x->x_ultima_img == maxim) x->x_ultima_img = 0;

            sprintf(nNstr, "%d", nN);
            strcat(ig_path,nNstr);
            strcat(ig_path,".");
            strcat(ig_path,FORMAT_MINIATURA);

            sys_vgui("image create photo img%x%d -file %s\n",x,nN,ig_path);
            sys_vgui(".x%x.c create image %d %d -image img%x%d -tags %xS%d\n",
                glist_getcanvas(x->x_glist),
                text_xpix(&x->x_obj, x->x_glist) + getX(x,nN) + (x->x_w_cell/2),
                text_ypix(&x->x_obj, x->x_glist) + getY(x,nN) + (x->x_h_cell/2),
                x,nN,x,nN);
            if(nN == 0){
                x->x_tauler_primer = x->x_cua.final;
            }
        }
    }

    /* v 0.2 -- mètode de la classe que marca la cel·la seleccionada */
    static void videogrid_grasp_selected(t_videogrid *x, int pos)
    {
        /*printf("Grasp selected thumb %d", pos);*/
        if(pos != x->x_pos_selected) {
            videogrid_ungrasp_selected(x);
            /* post("Grasp selected thumb %d", pos); */
            x->x_pos_selected = pos;
            /* nem per aqui ---- */
            sys_vgui(".x%x.c create rectangle %d %d %d %d -fill {} -tags %xGRASP -outline %s -width %d\n",
                    glist_getcanvas(x->x_glist),
                    text_xpix(&x->x_obj, x->x_glist) + getX(x,x->x_pos_selected), text_ypix(&x->x_obj, x->x_glist) + getY(x,x->x_pos_selected),
                    text_xpix(&x->x_obj, x->x_glist) + getX(x,x->x_pos_selected) + x->x_w_cell, text_ypix(&x->x_obj, x->x_glist) + getY(x,x->x_pos_selected) + x->x_h_cell,
                    x,x->x_color_grasp->s_name, GRUIX + 1);
            canvas_fixlinesfor(glist_getcanvas(x->x_glist), (t_text*)x);
        }
    }

    /*  widget helper functions  */

    /* dibuixa videogrid */
    void videogrid_drawme(t_videogrid *x, t_glist *glist, int firsttime)
    {
        /* post("Entra a drawme amb firsttime: %d", firsttime); */
        if (firsttime) {
            char name[MAXPDSTRING];
            canvas_makefilename(glist_getcanvas(x->x_glist), x->x_name->s_name, name, MAXPDSTRING);
            sys_vgui(".x%x.c create rectangle %d %d %d %d -fill %s -tags %xGRID -outline %s\n",
                glist_getcanvas(glist),
                text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
                text_xpix(&x->x_obj, glist) + (x->x_num_col * x->x_w_cell) + 1 + (x->x_num_col * GRUIX) + GRUIX, text_ypix(&x->x_obj, glist) + (x->x_num_fil * x->x_h_cell) + 1 + (x->x_num_fil * GRUIX) + GRUIX,
                x->x_color_fons->s_name, x,x->x_color_marc->s_name);

            canvas_fixlinesfor(glist_getcanvas(glist), (t_text*)x);
            /* si hi elements a la cua els afegeix (redimensió) */
            if(!cuaBuida(&x->x_cua))
            {
                path  ig_path;
                int nN = 0;
                char nNstr[BYTES_NUM_TEMP];
                Node *actual;
                actual=x->x_cua.davanter;
                do{
                    strcpy(ig_path,PATH_TEMPORAL);
                    sprintf(nNstr, "%d", nN);
                    strcat(ig_path,nNstr);
                    strcat(ig_path,".");
                    strcat(ig_path,FORMAT_MINIATURA);
                    /* post("reestablint la imatge %s", actual->pathFitxer); */
                    convertir_img_ff(actual->pathFitxer,FORMAT_MINIATURA, x->x_w_cell, x->x_h_cell, nN);
                    sys_vgui("image create photo img%x%d -file %s\n",x,nN,ig_path);
                    sys_vgui(".x%x.c create image %d %d -image img%x%d -tags %xS%d\n",
                             glist_getcanvas(x->x_glist),text_xpix(&x->x_obj, x->x_glist) + getX(x,nN) + (x->x_w_cell/2), text_ypix(&x->x_obj, x->x_glist) + getY(x,nN) + (x->x_h_cell/2),x,nN,x,nN);
                    actual = actual->seguent;
                    nN++;
                }while(actual);
            }
        }
        else {
            sys_vgui(".x%x.c coords %xGRID %d %d %d %d\n", glist_getcanvas(glist), x, text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),text_xpix(&x->x_obj, glist) + (x->x_num_col*x->x_w_cell) + 1 + (x->x_num_col * GRUIX) + GRUIX, text_ypix(&x->x_obj, glist) + (x->x_num_fil*x->x_h_cell) + 1 + (x->x_num_fil * GRUIX) + GRUIX);
            if(!cuaBuida(&x->x_cua))
            {
                int contador = 0;
                while(contador < numNodes(&x->x_cua)){
                    sys_vgui(".x%x.c coords %xS%d \
                            %d %d\n",
                    glist_getcanvas(glist), x, contador,
                    text_xpix(&x->x_obj, x->x_glist) + getX(x,contador) + (x->x_w_cell/2), text_ypix(&x->x_obj, x->x_glist) + getY(x,contador) + (x->x_h_cell/2));
                    contador++;
                }

               /* char buf[800];
                sprintf(buf, "pdtk_videogrid_table %%s %s %d %d\n", x->x_name->s_name, x->x_num_fil, x->x_num_col);
                gfxstub_new(&x->x_obj.ob_pd, x, buf); */
            }
            if (x->x_pos_selected > -1){
                sys_vgui(".x%x.c coords %xGRASP %d %d %d %d\n", glist_getcanvas(glist), x,
                text_xpix(&x->x_obj, x->x_glist) + getX(x,x->x_pos_selected), text_ypix(&x->x_obj, x->x_glist) + getY(x,x->x_pos_selected),
                text_xpix(&x->x_obj, x->x_glist) + getX(x,x->x_pos_selected) + x->x_w_cell, text_ypix(&x->x_obj, x->x_glist) + getY(x,x->x_pos_selected) + x->x_h_cell);
            }
            sys_vgui(".x%x.c delete %xLINIA\n", glist_getcanvas(x->x_glist), x);
        }
        int xI = text_xpix(&x->x_obj, glist);
        int yI = text_ypix(&x->x_obj, glist);
        int xF = xI + (x->x_num_col * x->x_w_cell) + ((x->x_num_col + 1) * GRUIX);
        int yF = yI + (x->x_num_fil * x->x_h_cell) + ((x->x_num_fil + 1) * GRUIX);
        int vlines = 0;
        int xi = 0;
        while(vlines < x->x_num_col){
            xi = xI + getX(x,vlines) - GRUIX + 1;
            sys_vgui(".x%x.c create line %d %d %d %d -fill %s -width %d -tag %xLINIA\n", glist_getcanvas(x->x_glist), xi, yI, xi, yF, x->x_color_marc->s_name,GRUIX,x);
            vlines++;
        }
        xi = xi + x->x_w_cell + GRUIX;
        sys_vgui(".x%x.c create line %d %d %d %d -fill %s -width %d -tag %xLINIA\n", glist_getcanvas(x->x_glist), xi, yI, xi, yF, x->x_color_marc->s_name,GRUIX,x);
        int hlines = 0;
        int yi = 0;
        while(hlines < x->x_num_fil){
            yi = yI + ((x->x_h_cell + GRUIX) * hlines) + 2;
            sys_vgui(".x%x.c create line %d %d %d %d -fill %s -width %d -tag %xLINIA\n", glist_getcanvas(x->x_glist), xI, yi, xF, yi, x->x_color_marc->s_name,GRUIX,x);
            hlines++;
        }
        yi = yi + x->x_h_cell + GRUIX;
        sys_vgui(".x%x.c create line %d %d %d %d -fill %s -width %d -tag %xLINIA\n", glist_getcanvas(x->x_glist), xI, yi, xF, yi, x->x_color_marc->s_name,GRUIX,x);
    }

    static void videogrid_delete(t_gobj *z, t_glist *glist)
    {
        /* post("Entra a delete"); */
        t_text *x = (t_text *)z;
        canvas_deletelinesfor(glist_getcanvas(glist), x);
    }

    static void videogrid_displace(t_gobj *z, t_glist *glist,int dx, int dy)
    {
        /* post("Entra a displace amb dx %d i dy %d", dx, dy); */
        t_videogrid *x = (t_videogrid *)z;
        x->x_obj.te_xpix += dx;
        x->x_obj.te_ypix += dy;
        sys_vgui(".x%x.c coords %xGRID %d %d %d %d\n",
                 glist_getcanvas(glist), x,
                 text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
                 text_xpix(&x->x_obj, glist) + (x->x_num_col*x->x_w_cell) + 1 + (x->x_num_col * GRUIX) + GRUIX, text_ypix(&x->x_obj, glist) + (x->x_num_fil*x->x_h_cell) + 1 + (x->x_num_fil * GRUIX) + GRUIX);
        videogrid_drawme(x, glist, 0);
        canvas_fixlinesfor(glist_getcanvas(glist),(t_text*) x);
    }

    /* borra videogrid v 0.2 -- int toclear */
    void videogrid_erase(t_videogrid* x,t_glist* glist, int toclear)
    {
        int maxim = x->x_num_fil * x->x_num_col;
        path path_total;
        char contador_str[BYTES_NUM_TEMP];
        /* post("Entra a erase"); */
        /* elimina les imatges */
        int contador = 0;
        while(contador < numNodes(&x->x_cua)){

            sys_vgui(".x%x.c delete %xS%d\n", glist_getcanvas(x->x_glist), x, contador);
            strcpy(path_total,PATH_TEMPORAL);
            sprintf(contador_str,"%d", contador);
            strcat(path_total,contador_str);
            strcat(path_total,".");
            strcat(path_total,FORMAT_MINIATURA);
            if(unlink(path_total)){
                /* post("Imatge temporal %s eliminada\n",path_total); */
            }
            contador++;
        }

        /* elimina el grid v 0.2 -- excepte quan es fa un clear */
        if(toclear == 0){
            sys_vgui(".x%x.c delete %xGRID\n", glist_getcanvas(glist), x);
            sys_vgui(".x%x.c delete %xLINIA\n", glist_getcanvas(x->x_glist), x);
        }
        /* v 0.2 -- elimina el marc de la casella seleccionada */
        if(x->x_pos_selected > -1){
            sys_vgui(".x%x.c delete %xGRASP\n", glist_getcanvas(glist), x);
            x->x_pos_selected = -1;
        }
        eliminar_imatges_temporals(maxim);
    }

    static void videogrid_vis(t_gobj *z, t_glist *glist, int vis)
    {
        /* post("Entra a vist amb vis %d", vis); */
        t_videogrid* s = (t_videogrid*)z;
        if (vis)
            videogrid_drawme(s, glist, 1);
        else
           videogrid_erase(s,glist,0);
    }

    static void videogrid_select(t_gobj *z, t_glist *glist, int state)
    {
        /* post("Entra select amb state %d", state); */
        t_videogrid *x = (t_videogrid *)z;
        if (state) {
            /* post("Videogrid seleccionat"); */
            sys_vgui(".x%x.c itemconfigure %xGRID -outline #0000FF\n", glist_getcanvas(glist), x);
        }
        else {
            /* post("Videogrid deseleccionat"); */
            sys_vgui(".x%x.c itemconfigure %xGRID -outline %s\n", glist_getcanvas(glist), x, x->x_color_marc->s_name);
        }
    }

    static void videogrid_getrect(t_gobj *z, t_glist *glist,int *xp1, int *yp1, int *xp2, int *yp2)
    {
        int cols, fils;
        t_videogrid* x = (t_videogrid*)z;
        cols = x->x_num_col;
        fils = x->x_num_fil;
        *xp1 = text_xpix(&x->x_obj, glist);
        *yp1 = text_ypix(&x->x_obj, glist);
        *xp2 = text_xpix(&x->x_obj, glist) + (cols*x->x_w_cell) + ((cols + 1) * GRUIX);
        *yp2 = text_ypix(&x->x_obj, glist) + (fils*x->x_h_cell) + ((fils + 1) * GRUIX);
        /* post("Esta amb el ratoli en el punt %d %d %d %d o son els vetexs de la caixa... es/bd", xp1, yp1, xp2, yp2); */
    }


    static void videogrid_save(t_gobj *z, t_binbuf *b)
    {
        /* post("Entra a save"); */
        t_videogrid *x = (t_videogrid *)z;
        /* crea la cadena de paths per desar */
        /* 100 possibles paths com a màxim a 512 cada path*/
        /* char cadenaPaths[51200];*/
        char *cadenaPaths, *cadenaPathsInicials;
        path ultimPath = "";
        cadenaPaths = (char *)malloc(51200*sizeof(char));
        strcpy(cadenaPaths,"");
        cadenaPathsInicials = (char *)malloc(51200*sizeof(char));
        strcpy(cadenaPathsInicials,"");
        /*strcpy(cadenaPaths,(char *)argv[5].a_w.w_symbol->s_name);*/
        if(!cuaBuida(&x->x_cua))
        {
            Node *actual;
            int maxim = x->x_num_fil * x->x_num_col;
            int contador = x->x_ultima_img + 1;

            if (contador > maxim) {
                   contador = 0;
            }
            /* printf("\n contador %d i maxim %d i laultimaPOS %d \n", contador, maxim, x->x_ultima_img); */
            /*
            strcat(cadenaPaths, actual->pathFitxer);
            strcat(cadenaPaths, "1|\n");
            contador ++;
            */
            /* prenem el davanter de la cua */
            actual=x->x_cua.davanter;

            while(contador < numNodes(&x->x_cua)){
                /* afegim els paths del davanter fins a l'ultim node al tauler */
                strcat(cadenaPaths, actual->pathFitxer);
                strcat(cadenaPaths, "|");
                actual = actual->seguent;
                contador ++;
            }
            if(actual != x->x_cua.final){
                /* ara resten els de de l'inici del tauler fins al final de la cua */
                while(actual != x->x_cua.final){
                    strcat(cadenaPathsInicials, actual->pathFitxer);
                    strcat(cadenaPathsInicials, "|");
                    actual = actual->seguent;
                }
                /* afegeix l'últim */
                strcat(ultimPath, actual->pathFitxer);
                strcat(ultimPath, "|");
                /* afegeix l'ultim de la cua */
                strcat(cadenaPathsInicials, ultimPath);
            }else{
                if(x->x_ultima_img == 0){
                    strcat(ultimPath, actual->pathFitxer);
                    strcat(ultimPath, "|");
                    strcat(cadenaPathsInicials, ultimPath);
                }
            }
            /* ordena el paths segons aparicio en el tauler */
            strcat(cadenaPathsInicials, cadenaPaths);
            /* DE MOMENT NO DESA ELS PATHS */
            strcat(cadenaPathsInicials, "");
            /* printf("%s",cadenaPathsInicials); */
        }

        binbuf_addv(b, "ssiissiissssiis", gensym("#X"),gensym("obj"),
        x->x_obj.te_xpix, x->x_obj.te_ypix,
        atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
        x->x_name,x->x_num_fil,x->x_num_col,x->x_color_fons,x->x_color_marc,x->x_color_grasp,x->x_format_list,x->x_w_cell,x->x_h_cell,gensym(cadenaPathsInicials));
        binbuf_addv(b, ";");
    }

    static void videogrid_properties(t_gobj *z, t_glist *owner)
    {
        char buf[900];
        t_videogrid *x=(t_videogrid *)z;

        /* post("Es crida a pdtk_videogrid dialog passant nom = %s\n fils = %d \t cols = %d \t color fons = %s \t color marc = %s\n", x->x_name->s_name, x->x_num_fil, x->x_num_col, x->x_color_fons->s_name, x->x_color_marc->s_name); */
        sprintf(buf, "pdtk_videogrid_dialog %%s %s %d %d %s %s %s %s %i %i\n",
                x->x_name->s_name, x->x_num_fil, x->x_num_col, x->x_color_fons->s_name, x->x_color_marc->s_name, x->x_color_grasp->s_name, x->x_format_list->s_name, x->x_w_cell,x->x_h_cell);
        /* post("videogrid_properties : %s", buf ); */
        gfxstub_new(&x->x_obj.ob_pd, x, buf);
    }

    static void videogrid_dialog(t_videogrid *x, t_symbol *s, int argc, t_atom *argv)
    {
        int maxim, maxdigit;
        int nfil = 0;
        int ncol = 0;
        if ( !x ) {
            post("Videogrid: error_ Attempt to alter the properties of an object that does not exist.\n");
        }
        switch (argc) {
        case 3:
            /* versio inicial */
            if (argv[0].a_type != A_SYMBOL || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT)
            {
                post("Videogrid: error_ Some of the values are inconsistent in its data type.\n");
                return;
            }
            x->x_name = argv[0].a_w.w_symbol;
            nfil = (int)argv[1].a_w.w_float;
            ncol = (int)argv[2].a_w.w_float;
        break;
        case 5:
            /* versio 0.1 */
            if (argv[0].a_type != A_SYMBOL || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT || argv[3].a_type != A_SYMBOL || argv[4].a_type != A_SYMBOL)
            {
                post("Videogrid: error_ Some of the values are inconsistent in its data type.\n");
                return;
            }
            x->x_name = argv[0].a_w.w_symbol;
            nfil = (int)argv[1].a_w.w_float;
            ncol = (int)argv[2].a_w.w_float;
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_format_list = gensym("mov:mpg");
            x->x_w_cell = 60;
            x->x_h_cell = 40;
        break;

        case 6:
            /* versio 0.2 */
            if (argv[0].a_type != A_SYMBOL || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT || argv[3].a_type != A_SYMBOL || argv[4].a_type != A_SYMBOL || argv[5].a_type != A_SYMBOL)
            {
                post("Videogrid: error_ Some of the values are inconsistent in its data type.\n");
                return;
            }
            x->x_name = argv[0].a_w.w_symbol;
            nfil = (int)argv[1].a_w.w_float;
            ncol = (int)argv[2].a_w.w_float;
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_color_grasp = argv[5].a_w.w_symbol;
            x->x_format_list = gensym("mov:mpg");
            x->x_w_cell = 60;
            x->x_h_cell = 40;
        break;

        case 7:
            /* versio 0.2.1 */
            if (argv[0].a_type != A_SYMBOL || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT || argv[3].a_type != A_SYMBOL || argv[4].a_type != A_SYMBOL || argv[5].a_type != A_SYMBOL || argv[6].a_type != A_SYMBOL)
            {
                post("Videogrid: error_ Some of the values are inconsistent in its data type.\n");
                return;
            }
            x->x_name = argv[0].a_w.w_symbol;
            nfil = (int)argv[1].a_w.w_float;
            ncol = (int)argv[2].a_w.w_float;
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_color_grasp = argv[5].a_w.w_symbol;
            x->x_format_list = argv[6].a_w.w_symbol;
            x->x_w_cell = 60;
            x->x_h_cell = 40;
        break;

        case 9:
            /* versio 0.2.2 */
            if (argv[0].a_type != A_SYMBOL || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT || argv[3].a_type != A_SYMBOL || argv[4].a_type != A_SYMBOL || argv[5].a_type != A_SYMBOL || argv[6].a_type != A_SYMBOL || argv[7].a_type != A_FLOAT || argv[8].a_type != A_FLOAT)
            {
                    post("Videogrid: error_ Some of the values are inconsistent in its data type.\n");
                    return;
            }
            x->x_name = argv[0].a_w.w_symbol;
            nfil = (int)argv[1].a_w.w_float;
            ncol = (int)argv[2].a_w.w_float;
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_color_grasp = argv[5].a_w.w_symbol;
            x->x_format_list = argv[6].a_w.w_symbol;
            x->x_w_cell = (int)argv[7].a_w.w_float;
            x->x_h_cell = (int)argv[8].a_w.w_float;
        break;

        default:
            /* no fa res */
        break;
        }
        /* amb aquest nom es prepara per poder rebre dades */
        pd_bind(&x->x_obj.ob_pd, x->x_name);
        /* el màxim es fixa pel nombre de digits utilitzats pel nom de la imatge temporal */
        maxdigit = pow(10,BYTES_NUM_TEMP);
        if((nfil*ncol) <= maxdigit){
            if((nfil*ncol) > 0){
                x->x_num_fil = nfil;
                x->x_num_col = ncol;
            }else{
                post("Videogrid: The number of rows and columns is less than the minimum allowed: 1 cell.\n");
            }
        }else{
            post("Videogrid: The number of rows and columns exceeds the maximum allowed: a total of %d cells.\n",maxdigit);
        }
        /* post("Videogrid: Modified values\n name = %s\n rows = %d \t cols = %d.\n", x->x_name->s_name, x->x_num_fil, x->x_num_col); */
        /* elimina els nodes no representables amb la nova configuració */
        maxim = x->x_num_fil * x->x_num_col;
        int extret;
        videogrid_erase(x, x->x_glist,0);
        /* si hi ha més nodes a la cua que el maxim */
        while((numNodes(&x->x_cua)) >  maxim){
            /* desencuem */
            extret = desencuar(&x->x_cua);
        }
        /* al reestablir el tamany del tauler cal saber la posició de l'últim element */
        // x->x_ultima_img = numNodes(&x->x_cua) - 1;
        x->x_ultima_img = numNodes(&x->x_cua);
        if (x->x_ultima_img <  0) x->x_ultima_img = 0;
        x->x_tauler_primer = x->x_cua.davanter;
        videogrid_drawme(x, x->x_glist, 1);
    }

    /* v 0.2 -- mètode de la classe que dispara l'element del taules en la posicio N [seek N( */
    void videogrid_seek(t_videogrid *x, t_floatarg postauler)
    {
        /* post("seek a %d\n",postauler); */
        path pathSortida;
        Node *actual;
        int contador = 0;
        int maxim = x->x_num_fil * x->x_num_col;
        /* obtenir el path per enviar a la sortida */
        if((!cuaBuida(&x->x_cua))&&(postauler < numNodes(&x->x_cua))&&(postauler >= 0 )){
            if(x->x_tauler_primer){
                actual = x->x_tauler_primer;
                while(contador <= postauler){
                    if(contador == postauler){
                        strcpy(pathSortida,actual->pathFitxer);
                    }
                    if(actual->seguent == NULL){
                        actual = x->x_cua.davanter;
                    }else{
                        actual = actual->seguent;
                    }
                    contador++;
                }
                outlet_symbol(x->x_sortida, gensym(pathSortida));
                /* post("Esta a videogrid_click amb %d %d a la posicio %d\n", x_pos, y_pos, postauler);*/
                /* v 0.2 -- marcar casella */
                videogrid_grasp_selected(x, postauler);
            }
        }
    }

    static int videogrid_click(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit)
    {
        t_videogrid* x = (t_videogrid *)z;
        int x_pos = xpix - text_xpix(&x->x_obj, x->x_glist);
        int y_pos = ypix - text_ypix(&x->x_obj, x->x_glist);
        int xa, ya, postauler;
        if (doit)
        {
            /* obtenir la posicio en el tauler */
            // -- v 0.2 -- midoficacio pel gruix del marc //
            xa = ((x_pos) / (x->x_w_cell + GRUIX + 1));
            ya = ((y_pos) / (x->x_h_cell + GRUIX + 1)) * x->x_num_col;
            postauler = ya + xa;
            // -- v 0.2 -- seleciona la casella disparant el path //
            videogrid_seek(x, postauler);
        }
        return (1);
    }

    /* --------- videogrid functions ---------- */

    /* v 0.2 -- mètode de la classe que buida el tauler amb el missatge [clear ( */
    void videogrid_clear(t_videogrid *x)
    {
        videogrid_erase(x, x->x_glist,1);
        eliminarCua(&x->x_cua);
        x->x_ultima_img = 0;
        x->x_dir_pos = 0;
        x->x_tauler_primer = NULL;
        videogrid_drawme(x, x->x_glist, 0);
    }

    /* mètode de la clase que escriu un missatge al rebre un bang */
    void videogrid_bang(t_videogrid *x)
    {
        /* post("Hello videogrid !!"); */
        escriuCua(&x->x_cua);
    }

    /* mètode de la classe que es dispara al rebre una entrada de missatge amb [putvideo +string( com a paràmetre */
    void videogrid_putvideo(t_videogrid *x, t_symbol *entrada)
    {
        /* comprova que existeixi el fitxer */
        FILE *fitxer;
        path e;
        strcpy(e,entrada->s_name);
        fitxer = fopen(e,"r");
        if (!fitxer) {
            post("Videogrid: Problem opening file %s.\n",e);
        }
        else {
            if (format_adequat_v(e, x->x_format_list) != 0) {
                videogrid_afegir_imatge(x,e);
            }
        }
    }

    /* mètode de la classe que es dispara al rebre una entrada de missatge amb [putvideodir +string( com a paràmetre */
    void *videogrid_putvideodir_thread(void *z)
    {
        t_videogrid *x = (t_videogrid *)z;
        DIR *dirp;
        struct dirent * direntp;
        path nomImatge, directoriAnterior, pathActual;
        int numEncuats = 0, numPosDir = 0;
        int maxim;
        if ((dirp = opendir(x->x_dir_canvi)) == NULL)
        {
            post("Videogrid: Can not open folder %s.\n", x->x_dir_canvi);
        }else{
            maxim = x->x_num_fil * x->x_num_col;
            strcpy(directoriAnterior, x->x_dir_actual);
            strcpy(x->x_dir_actual, x->x_dir_canvi);
            /*
             * si es el mateix directori entrat l'ultim busca la ultima imatge afegida
             * per a seguir a encuant a partir d'ella en endavant
             */
            if(strcmp(directoriAnterior, x->x_dir_actual) == 0){
                /* post("Videogrid: Repeteix directori %s\n", x->x_dir_actual); */
                while ( (direntp = readdir( dirp )) != NULL ){
                    /* es descarta el mateix directori, el directori anterior i tot el que no sigui un fitxer regular */
                    if((strcmp(direntp->d_name,"..") != 0)&&(strcmp(direntp->d_name,".") != 0)&&(direntp->d_type == DT_REG)){
                        /* incrementa la posició en el directori */
                        numPosDir++;
                        /* assolir la posició anterior en el directori */
                        if(numPosDir > x->x_dir_pos){
                            /* si el nombre de nodes encuats per aquest directori no supera el màxim encua el nou node */
                            if(numEncuats < maxim){
                                /* post("s'encua la imatge %s\n", direntp->d_name); */
                                /* concatena el path i el nom de la imatge */
                                strcpy(nomImatge,direntp->d_name);
                                strcpy(pathActual,x->x_dir_actual);
                                strcat(pathActual,"/");
                                strcat(pathActual,nomImatge);
                                if (format_adequat_v(pathActual, x->x_format_list) != 0) {
                                    pthread_mutex_lock(&x->x_lock);
                                    videogrid_afegir_imatge(x, pathActual);
                                    pthread_mutex_unlock(&x->x_lock);
                                    /* incrementa en 1 per indicar el nombre de nodes encuats per aquest directori */
                                    numEncuats++;
                                }
                                /* es desa la posició en el directori de l'últim node encuat */
                                x->x_dir_pos = numPosDir;
                            }
                        }
                    }
                }
            }else{
                /* directori diferent omple la cua començant pel primer fitxer */
                /* post("Videogrid: Nou directori %s \n", x->x_dir_actual); */
                while ( (direntp = readdir( dirp )) != NULL ){
                    /* es descarta el mateix directori, el directori anterior i tot el que no sigui un fitxer regular */
                    if((strcmp(direntp->d_name,"..") != 0)&&(strcmp(direntp->d_name,".") != 0)&&(direntp->d_type == DT_REG)){
                        /* incrementa la posició en el directori */
                        numPosDir++;
                        /* si el nombre de nodes encuats per aquest directori no supera el màxim enca el nou node */
                        if(numEncuats < maxim){
                            /* post("s'encua la imatge %s\n", direntp->d_name); */
                            /* concatena el path i el nom de la imatge */
                            strcpy(nomImatge,direntp->d_name);
                            strcpy(pathActual,x->x_dir_actual);
                            strcat(pathActual,"/");
                            strcat(pathActual,nomImatge);
                            if (format_adequat_v(pathActual, x->x_format_list) != 0) {
                                pthread_mutex_lock(&x->x_lock);
                                videogrid_afegir_imatge(x, pathActual);
                                pthread_mutex_unlock(&x->x_lock);
                                /* incrementa en 1 per indicar el nombre de nodes encuats per aquest directori */
                                numEncuats++;
                            }
                            /* es desa la posició en el directori de l'últim node encuat */
                            x->x_dir_pos = numPosDir;
                        }
                    }
                }
            }
            /* si la posicio de l'actual es la de l'utim fitxer del directori, inicialitza la posició */
            if(x->x_dir_pos >= numPosDir) x->x_dir_pos = 0;
            closedir(dirp);
        }
        /* escriu l'argument entrat */
        /* post("Obtenint imatges del directori: %s ...",x->x_dir_canvi); */
        /* envia a la sorida l'argument entrat */
        /* outlet_symbol(x->x_sortida, entrada); */
        pthread_exit(NULL);
    }

    void videogrid_putvideodir(t_videogrid *x, t_symbol *entrada)
    {

        pthread_t unthread;
        pthread_attr_t unatribut;
        pthread_attr_init( &unatribut );

        strcpy(x->x_dir_canvi,entrada->s_name);

        // ----------------      THREAD CREAT -------------------------
        pthread_mutex_init(&x->x_lock, NULL);
        // int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
        pthread_create(&unthread, &unatribut, videogrid_putvideodir_thread, (void *)x);
        pthread_mutex_destroy(&x->x_lock);
    }

    /* v 0.2.3 -- mètode de la classe que modifica el nombre de files del tauler [rows N( */
    void videogrid_rows(t_videogrid *x, t_floatarg nfil)
    {
        /* post("rows a %d\n",postauler); */
        int maxim, maxdigit, ncol;
        /* el màxim es fixa pel nombre de digits utilitzats pel nom de la imatge temporal */
        maxdigit = pow(10,BYTES_NUM_TEMP);
        ncol = x->x_num_col;
        if((nfil*ncol) <= maxdigit){
            if((nfil*ncol) > 0){
                x->x_num_fil = nfil;
                /* x->x_num_col = ncol; */
            }else{
                post("Videogrid: The number of rows and columns is less than the minimum allowed: 1 cell.\n");
            }
        }else{
            post("Videogrid: The number of rows and columns exceeds the maximum allowed: a total of %d cells.\n",maxdigit);
        }
        /* post("Videogrid: Modified values\n name = %s\n rows = %d \t cols = %d.\n", x->x_name->s_name, x->x_num_fil, x->x_num_col); */
        /* elimina els nodes no representables amb la nova configuració */
        maxim = x->x_num_fil * x->x_num_col;
        int extret;
        videogrid_erase(x, x->x_glist,0);
        /* si hi ha més nodes a la cua que el maxim */
        while((numNodes(&x->x_cua)) >  maxim){
            /* desencuem */
            extret = desencuar(&x->x_cua);
        }
        /* al reestablir el tamany del tauler cal saber la posició de l'últim element */
        x->x_ultima_img = numNodes(&x->x_cua) - 1;
        if (x->x_ultima_img <  0) x->x_ultima_img = 0;
        x->x_tauler_primer = x->x_cua.davanter;
        videogrid_drawme(x, x->x_glist, 1);
    }

    /* v 0.2.3 -- mètode de la classe que modifica el nombre de columnes del tauler [cols N( */
    void videogrid_cols(t_videogrid *x, t_floatarg ncol)
    {
        /* post("rows a %d\n",postauler); */
        /* post("rows a %d\n",postauler); */
        int maxim, maxdigit, nfil;
        /* el màxim es fixa pel nombre de digits utilitzats pel nom de la imatge temporal */
        maxdigit = pow(10,BYTES_NUM_TEMP);
        nfil = x->x_num_fil;
        if((nfil*ncol) <= maxdigit){
            if((nfil*ncol) > 0){
                /* x->x_num_fil = nfil; */
                x->x_num_col = ncol;
            }else{
                post("Videogrid: The number of rows and columns is less than the minimum allowed: 1 cell.\n");
            }
        }else{
            post("Videogrid: The number of rows and columns exceeds the maximum allowed: a total of %d cells.\n",maxdigit);
        }
        /* post("Videogrid: Modified values\n name = %s\n rows = %d \t cols = %d.\n", x->x_name->s_name, x->x_num_fil, x->x_num_col); */
        /* elimina els nodes no representables amb la nova configuració */
        maxim = x->x_num_fil * x->x_num_col;
        int extret;
        videogrid_erase(x, x->x_glist,0);
        /* si hi ha més nodes a la cua que el maxim */
        while((numNodes(&x->x_cua)) >  maxim){
            /* desencuem */
            extret = desencuar(&x->x_cua);
        }
        /* al reestablir el tamany del tauler cal saber la posició de l'últim element */
        x->x_ultima_img = numNodes(&x->x_cua) - 1;
        if (x->x_ultima_img <  0) x->x_ultima_img = 0;
        x->x_tauler_primer = x->x_cua.davanter;
        videogrid_drawme(x, x->x_glist, 1);
    }

    /* tk help windows */

    void load_tk_procs_videogrid () {
        // ########### procediments per videogrid -- slario(at)gmail.com [a partir del codi del grid de l'Ives: ydegoyon(at)free.fr] #########
        sys_gui("proc videogrid_apply {id} {\n");
        // strip "." from the TK id to make a variable name suffix
        sys_gui("set vid [string trimleft $id .]\n");
        // for each variable, make a local variable to hold its name...
        sys_gui("set var_graph_name [concat graph_name_$vid]\n");
        sys_gui("global $var_graph_name\n");
        sys_gui("set var_graph_num_fil [concat graph_num_fil_$vid]\n");
        sys_gui("global $var_graph_num_fil\n");
        sys_gui("set var_graph_num_col [concat graph_num_col_$vid]\n");
        sys_gui("global $var_graph_num_col\n");
        sys_gui("set var_graph_color_fons [concat graph_color_fons_$vid]\n");
        sys_gui("global $var_graph_color_fons\n");
        sys_gui("set var_graph_color_marc [concat graph_color_marc_$vid]\n");
        sys_gui("global $var_graph_color_marc\n");
        sys_gui("set var_graph_color_grasp [concat graph_color_grasp_$vid]\n");
        sys_gui("global $var_graph_color_grasp\n");
        sys_gui("set var_graph_format_list [concat graph_format_list_$vid]\n");
        sys_gui("global $var_graph_format_list\n");
        sys_gui("set var_graph_w_cell [concat graph_w_cell_$vid]\n");
        sys_gui("global $var_graph_w_cell\n");
        sys_gui("set var_graph_h_cell [concat graph_h_cell_$vid]\n");
        sys_gui("global $var_graph_h_cell\n");
        sys_gui("set cmd [concat $id dialog [eval concat $$var_graph_name] [eval concat $$var_graph_num_fil] [eval concat $$var_graph_num_col] [eval concat $$var_graph_color_fons] [eval concat $$var_graph_color_marc] [eval concat $$var_graph_color_grasp] [eval concat $$var_graph_format_list] [eval concat $$var_graph_w_cell] [eval concat $$var_graph_h_cell]\\;]\n");
        // puts stderr $cmd
        sys_gui("pd $cmd\n");
        sys_gui("}\n");
        sys_gui("proc videogrid_cancel {id} {\n");
        sys_gui("set cmd [concat $id cancel \\;]\n");
        // puts stderr $cmd
        sys_gui("pd $cmd\n");
        sys_gui("}\n");
        sys_gui("proc videogrid_ok {id} {\n");
        sys_gui("videogrid_apply $id\n");
        sys_gui("videogrid_cancel $id\n");
        sys_gui("}\n");
        sys_gui("proc pdtk_videogrid_dialog {id name num_fil num_col color_fons color_marc color_grasp format_list w_cell h_cell } {\n");
        sys_gui("set vid [string trimleft $id .]\n");
        sys_gui("set var_graph_name [concat graph_name_$vid]\n");
        sys_gui("global $var_graph_name\n");
        sys_gui("set var_graph_num_fil [concat graph_num_fil_$vid]\n");
        sys_gui("global $var_graph_num_fil\n");
        sys_gui("set var_graph_num_col [concat graph_num_col_$vid]\n");
        sys_gui("global $var_graph_num_col\n");
        sys_gui("set var_graph_color_fons [concat graph_color_fons_$vid]\n");
        sys_gui("global $var_graph_color_fons\n");
        sys_gui("set var_graph_color_marc [concat graph_color_marc_$vid]\n");
        sys_gui("global $var_graph_color_marc\n");
        sys_gui("set var_graph_color_grasp [concat graph_color_grasp_$vid]\n");
        sys_gui("global $var_graph_color_grasp\n");
        sys_gui("set var_graph_format_list [concat graph_format_list_$vid]\n");
        sys_gui("global $var_graph_format_list\n");
        sys_gui("set var_graph_w_cell [concat graph_w_cell_$vid]\n");
        sys_gui("global $var_graph_w_cell\n");
        sys_gui("set var_graph_h_cell [concat graph_h_cell_$vid]\n");
        sys_gui("global $var_graph_h_cell\n");
        sys_gui("set $var_graph_name $name\n");
        sys_gui("set $var_graph_num_fil $num_fil\n");
        sys_gui("set $var_graph_num_col $num_col\n");
        sys_gui("set $var_graph_color_fons $color_fons\n");
        sys_gui("set $var_graph_color_marc $color_marc\n");
        sys_gui("set $var_graph_color_grasp $color_grasp\n");
        sys_gui("set $var_graph_format_list $format_list\n");
        sys_gui("set $var_graph_w_cell $w_cell\n");
        sys_gui("set $var_graph_h_cell $h_cell\n");
        sys_gui("toplevel $id -class [winfo class .]\n");
        sys_gui("wm title $id {** videogrid **}\n");
        sys_gui("wm resizable $id 0 0\n");
        sys_gui("wm protocol $id WM_DELETE_WINDOW [concat videogrid_cancel $id]\n");

        /* sys_gui("label $id.label -text {VIDEOGRID PROPERTIES}\n"); */
        /* sys_gui("pack $id.label -side top\n"); */

        sys_gui("label $id.label -text { properties }\n");
        sys_gui("pack $id.label -side top\n");

        sys_gui("frame $id.buttonframe\n");
        sys_gui("pack $id.buttonframe -side bottom -fill x -pady 2m\n");
        sys_gui("button $id.buttonframe.cancel -text {Cancel} -command \"videogrid_cancel $id\"\n");
        sys_gui("button $id.buttonframe.apply -text {Apply} -command \"videogrid_apply $id\"\n");
        sys_gui("button $id.buttonframe.ok -text {OK} -command \"videogrid_ok $id\"\n");
        sys_gui("pack $id.buttonframe.cancel -side left -expand 1\n");
        sys_gui("pack $id.buttonframe.apply -side left -expand 1\n");
        sys_gui("pack $id.buttonframe.ok -side left -expand 1\n");

        sys_gui("frame $id.1rangef\n");
        sys_gui("pack $id.1rangef -side top\n");
        sys_gui("label $id.1rangef.lname -text \"Name :\" -anchor nw\n");
        sys_gui("entry $id.1rangef.name -textvariable $var_graph_name -width 20\n");
        sys_gui("pack $id.1rangef.lname $id.1rangef.name -side left\n");

        sys_gui("frame $id.2rangef\n");
        sys_gui("pack $id.2rangef -side top\n");
        sys_gui("label $id.2rangef.lnum_fil -text \"Rows :\" -width 15 -anchor nw\n");
        sys_gui("entry $id.2rangef.num_fil -textvariable $var_graph_num_fil -width 10\n");
        sys_gui("pack $id.2rangef.lnum_fil $id.2rangef.num_fil -side left\n");

        sys_gui("frame $id.3rangef\n");
        sys_gui("pack $id.3rangef -side top\n");
        sys_gui("label $id.3rangef.lnum_col -text \"Cols :\" -width 15 -anchor nw\n");
        sys_gui("entry $id.3rangef.num_col -textvariable $var_graph_num_col -width 10\n");
        sys_gui("pack $id.3rangef.lnum_col $id.3rangef.num_col -side left\n");

        sys_gui("frame $id.4rangef\n");
        sys_gui("pack $id.4rangef -side top\n");
        sys_gui("label $id.4rangef.lcolor_fons -text \"Bg Color :\" -width 15 -anchor nw\n");
        sys_gui("entry $id.4rangef.color_fons -textvariable $var_graph_color_fons -width 10\n");
        sys_gui("pack $id.4rangef.lcolor_fons $id.4rangef.color_fons -side left\n");

        sys_gui("frame $id.5rangef\n");
        sys_gui("pack $id.5rangef -side top\n");
        sys_gui("label $id.5rangef.lcolor_marc -text \"Border Color :\" -width 15 -anchor nw\n");
        sys_gui("entry $id.5rangef.color_marc -textvariable $var_graph_color_marc -width 10\n");
        sys_gui("pack $id.5rangef.lcolor_marc $id.5rangef.color_marc -side left\n");

        sys_gui("frame $id.6rangef\n");
        sys_gui("pack $id.6rangef -side top\n");
        sys_gui("label $id.6rangef.lcolor_grasp -text \"Sel Color :\" -width 15 -anchor nw\n");
        sys_gui("entry $id.6rangef.color_grasp -textvariable $var_graph_color_grasp -width 10\n");
        sys_gui("pack $id.6rangef.lcolor_grasp $id.6rangef.color_grasp -side left\n");

        sys_gui("frame $id.7rangef\n");
        sys_gui("pack $id.7rangef -side top\n");
        sys_gui("label $id.7rangef.lformat_list -text \"Format list ':' separated :\" -width 32 -anchor nw\n");
        sys_gui("entry $id.7rangef.format_list -textvariable $var_graph_format_list -width 30\n");
        sys_gui("pack $id.7rangef.lformat_list -fill x\n");
        sys_gui("pack $id.7rangef.format_list -fill x\n");

        sys_gui("frame $id.8rangef\n");
        sys_gui("pack $id.8rangef -side top\n");
        sys_gui("label $id.8rangef.lw_cell -text \"Thumb W :\" -width 15 -anchor nw\n");
        sys_gui("entry $id.8rangef.w_cell -textvariable $var_graph_w_cell -width 10\n");
        sys_gui("pack $id.8rangef.lw_cell $id.8rangef.w_cell -side left\n");

        sys_gui("frame $id.9rangef\n");
        sys_gui("pack $id.9rangef -side top\n");
        sys_gui("label $id.9rangef.lh_cell -text \"Thumb H :\" -width 15 -anchor nw\n");
        sys_gui("entry $id.9rangef.h_cell -textvariable $var_graph_h_cell -width 10\n");
        sys_gui("pack $id.9rangef.lh_cell $id.9rangef.h_cell -side left\n");

        sys_gui("bind $id.1rangef.name <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("bind $id.2rangef.num_fil <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("bind $id.3rangef.num_col <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("bind $id.4rangef.color_fons <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("bind $id.5rangef.color_marc <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("bind $id.6rangef.color_grasp <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("bind $id.7rangef.format_list <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("bind $id.8rangef.w_cell <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("bind $id.9rangef.h_cell <KeyPress-Return> [concat videogrid_ok $id]\n");
        sys_gui("focus $id.1rangef.name\n");
        sys_gui("}\n");

        /*
        sys_gui("proc table {w content args} {\n");
        sys_gui("frame $w -bg black\n");
        sys_gui("set r 0\n");
        sys_gui("foreach row $content {\n");
        sys_gui("set fields {}\n");
        sys_gui("set c 0\n");
        sys_gui("foreach col $row {\n");
        // lappend fields [label $w.$r/$c -text $col]
        sys_gui("set img [image create photo -file $col]\n");
        sys_gui("lappend fields [label $w.$r/$c -image $img]\n");
        sys_gui("incr c\n");
        sys_gui("}\n");
        sys_gui("eval grid $fields -sticky news -padx 1 -pady 1\n");
        sys_gui("incr r\n");
        sys_gui("}\n");
        sys_gui("set w\n");
        sys_gui("}\n");
        sys_gui("proc pdtk_videogrid_table {id name num_fil num_col} {\n");
        sys_gui("table .tauler {\n");
        sys_gui("{sll80x60.gif 3160x120.gif sll80x60.gif}\n");
        sys_gui("{sll80x60.gif sll80x60.gif sll80x60.gif}\n");
        sys_gui("{sll80x60.ppm sll80x60.gif 3160x120.gif}\n");
        sys_gui("}\n");
        sys_gui("pack .tauler\n");
        sys_gui("}\n");
        */
    }

    /* widget properties */
    static void videogrid_setwidget(void)
    {
        /* post("Entra a setwidget"); */
        videogrid_widgetbehavior.w_getrectfn = videogrid_getrect;
        videogrid_widgetbehavior.w_displacefn = videogrid_displace;
        videogrid_widgetbehavior.w_selectfn = videogrid_select;
        videogrid_widgetbehavior.w_activatefn = NULL;
        videogrid_widgetbehavior.w_deletefn = videogrid_delete;
        videogrid_widgetbehavior.w_visfn = videogrid_vis;
        /* clic del ratoli */
        videogrid_widgetbehavior.w_clickfn = videogrid_click;
    }

    /* el constructor de la classe*/
    static void *videogrid_new(t_symbol* name, int argc, t_atom *argv)
    {
        /* instanciació del nou objecte */
        t_videogrid *x = (t_videogrid *)pd_new(videogrid_class);
        /* crea una sortida per l'objecte*/
        x->x_sortida = outlet_new(&x->x_obj,&s_symbol);
        /* s'obté el canvas de pd */
        x->x_glist = (t_glist*) canvas_getcurrent();
        /* posició en el tauler de la última imatge afegida */
        x->x_ultima_img = 0;
        /* posició de l'últim fitxer del directori encuat */
        x->x_dir_pos = 0;
        /* apuntador al primer element en el tauler */
        x->x_tauler_primer = NULL;
        x->x_pos_selected = -1;
        /* fixa el nom de l'objecte */
        char nom[15];
        sprintf(nom, "videogrid%d", ++videogridcount);
        name = gensym(nom);
        x->x_name = name;
        /* amb aquest nom es prepara per poder rebre dades */
        pd_bind(&x->x_obj.ob_pd, x->x_name);
        /* crea la cua de nodes */
        crearCua(&x->x_cua);
        post("NEW videogrid: created with %d parameters.\n", argc);

        switch (argc) {
        case 3:
            /* versio inicial */
            x->x_num_fil = (int)atom_getintarg(1, argc, argv);
            x->x_num_col = (int)atom_getintarg(2, argc, argv);
            x->x_color_fons = gensym("#F0F0F0");
            x->x_color_marc = gensym("#0F0F0F");
            x->x_color_grasp = gensym("#F1882B");
            x->x_format_list = gensym("mov:mpg");
            x->x_w_cell = 60;
            x->x_h_cell = 40;
        break;
        case 5:
            /* versio 0.1 */
            x->x_num_fil = (int)atom_getintarg(1, argc, argv);
            x->x_num_col = (int)atom_getintarg(2, argc, argv);
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_color_grasp = gensym("#F1882B");
            x->x_format_list = gensym("mov:mpg");
            x->x_w_cell = 60;
            x->x_h_cell = 40;
        break;
        case 6:
            /* versio 0.2 */
            x->x_num_fil = (int)atom_getintarg(1, argc, argv);
            x->x_num_col = (int)atom_getintarg(2, argc, argv);
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_color_grasp = argv[5].a_w.w_symbol;
            x->x_format_list = gensym("mov:mpg");
            x->x_w_cell = 60;
            x->x_h_cell = 40;
        break;

        case 7:
            /* versio 0.2.1 */
            x->x_num_fil = (int)atom_getintarg(1, argc, argv);
            x->x_num_col = (int)atom_getintarg(2, argc, argv);
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_color_grasp = argv[5].a_w.w_symbol;
            x->x_format_list = argv[6].a_w.w_symbol;
            x->x_w_cell = 60;
            x->x_h_cell = 40;
        break;

        case 9:
            /* versio 0.2.2 */
            x->x_num_fil = (int)atom_getintarg(1, argc, argv);
            x->x_num_col = (int)atom_getintarg(2, argc, argv);
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_color_grasp = argv[5].a_w.w_symbol;
            x->x_format_list = argv[6].a_w.w_symbol;
            x->x_w_cell = (int)atom_getintarg(7, argc, argv);
            x->x_h_cell = (int)atom_getintarg(8, argc, argv);
        break;

        case 10:
            /* versio 0.1 - paths dels  elsements del tauler */
            x->x_num_fil = (int)atom_getintarg(1, argc, argv);
            x->x_num_col = (int)atom_getintarg(2, argc, argv);
            x->x_color_fons = argv[3].a_w.w_symbol;
            x->x_color_marc = argv[4].a_w.w_symbol;
            x->x_color_grasp = argv[5].a_w.w_symbol;
            x->x_format_list = gensym("mov:mpg");
            x->x_w_cell = 60;
            x->x_h_cell = 40;
            /*
            // -- llegir la cadena de paths | afegir els paths a la cua //
            // -- ATENCIO! NO AFEGEIX ELS PATHS !!! //
            char *cadenaPaths;
            cadenaPaths = (char *)malloc(51200*sizeof(char));
            strcpy(cadenaPaths,(char *)argv[6].a_w.w_symbol->s_name);
            // -- printf("Es carreguen els paths %s --- %s **** %s\n", cadenaPaths, argv[5].a_w.w_symbol->s_name,argv[3].a_w.w_symbol->s_name); //
            // -- split //
            char *token;
            t_symbol *tt;
            for ( token = strtok(argv[6].a_w.w_symbol->s_name,"|");
                token != NULL;
                token = strtok(NULL,"|") ){
                        tt = gensym(token);
                        // -- printf("AFEGINT CARREGANT %s\n",tt->s_name); //
                        // -- imagegrid_putimg(x,tt); //
                        // -- ATENCIO! NO AFEGEIX ELS PATHS !!! //
                        // -- imagegrid_afegir_imatge(x,tt->s_name); //
            }

            token = strtok(cadenaPaths,"|");
            while(token){
                tt = gensym(token);
                // printf("AFEGINT CARREGANT %s\n",tt->s_name);
                imagegrid_putimg(x,tt);
                token = strtok(NULL,"|");
            }
            free(cadenaPaths);
            */
        break;

        default:
            /* crea un objecte nou per defecte */
            /* post("NEW imagegrid created.\n"); */
            /* fixa el nombre de files */
            x->x_num_fil = 3;
            /* fixa el nombre de columnes */
            x->x_num_col = 5;
            /* colors de fons i de marc*/
            x->x_color_fons = gensym("#F0F0F0");
            x->x_color_marc = gensym("#0F0F0F");
            x->x_color_grasp = gensym("#F1882B");
            x->x_format_list = gensym("mov:mpg");
            x->x_w_cell = 60;
           x->x_h_cell = 40;
        break;
        }
        /*
         * printf("S'ha instanciat un videogrid anomenat %s amb les caracteristiques seguents:",x->x_name->s_name);
         * printf("Nombre de files %d - Nombre de columnes: %d", x->x_num_fil, x->x_num_col);
        */
        /* a Karmic es detecta que videogrid fa crash al pd al insertar un video, nomes en el cas que es carregui despres de Gem */


        char *lliibbGem = "Gem";
        char *lliibbVideogrid = "videogrid";
        /*
        if(sys_load_lib(glist_getcanvas(x->x_glist), lliibb)){
            post("SLL diu Gem already loaded");
        } else{
            post("SLL diu Gem not loaded");
        }*/

        int i, posGem = 0, posVideogrid = 0;
        t_namelist *nl;

        for (i=0, nl = sys_externlist; nl; i++, nl = nl->nl_next) {
            if(strcmp(nl->nl_string,lliibbGem)==0) {
                posGem = i;
            } else {
                if(strcmp(nl->nl_string,lliibbVideogrid)==0) {
                    posVideogrid = i;
                }
            }
        }
        if(posGem < posVideogrid) {
            post("NOTE videogrid: Recomended load before Gem.\n");
        }
        return (x);
    }

    static void videogrid_destroy(t_videogrid *x){
        /* allibera el nom */
        pd_unbind(&x->x_obj.ob_pd, x->x_name);
        /* elimina el contingut de la cua */
        eliminarCua(&x->x_cua);
        post("Videogrid destroyed.\n");
    }

    /* generacio d'una nova classe */
    /* al carregar la nova llibreria my_lib pd intenta cridar la funció my_lib_setup */
    /* aquesta crea la nova classe i les seves propietats només un sol cop */

    void videogrid_setup(void)
    {
        load_tk_procs_videogrid();
        post("videogrid: version 0.2.1");
        post("written by Sergi Lario (slario@gmail.com) & Lluis Gomez i Bigorda (lluis@artefacte.org)");


        videogrid_class = class_new(gensym("videogrid"),
            (t_newmethod)videogrid_new,
            (t_method)videogrid_destroy,
            sizeof(t_videogrid),
            CLASS_DEFAULT,
            A_GIMME,
            0);

        /*
         *  class_new crea la nova classe retornant un punter al seu prototipus,
         *  el primer argument es el nom simbolic de la classe,
         *  el segon i tercer corresponen al constructor i destructor de la classe respectivament,
         *  (el constructor instancia un objecte i inicialitza les seves dades cada cop que es crea un objecte
         *  el destructor allibera la memoria reservada al destruir l'objecte per qualsevol causa)
         *  el quart correspon a la mida de l'estructura de dades, per tal de poder reservar la memoria necessària,
         *  el cinquè influeix en el mòde de representació gràfica del objectes. Per defecte CLASS_DEFAULT o '0',
         *  la resta d'arguments defineixen els arguments de l'objecte i el seu tipus, la llista acaba  amb 0
        */

        class_addbang(videogrid_class, videogrid_bang);

        class_addmethod(videogrid_class, (t_method)videogrid_putvideo,gensym("putvideo"), A_DEFSYMBOL, 0);

        class_addmethod(videogrid_class, (t_method)videogrid_putvideodir,gensym("putvideodir"), A_DEFSYMBOL, 0);

        class_addmethod(videogrid_class, (t_method)videogrid_dialog, gensym("dialog"), A_GIMME, 0);

        class_addmethod(videogrid_class, (t_method)videogrid_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);

        class_addmethod(videogrid_class, (t_method)videogrid_clear, gensym("clear"), A_GIMME, 0);

        class_addmethod(videogrid_class, (t_method)videogrid_seek, gensym("seek"), A_FLOAT, 0);

        class_addmethod(videogrid_class, (t_method)videogrid_rows, gensym("rows"), A_FLOAT, 0);

        class_addmethod(videogrid_class, (t_method)videogrid_cols, gensym("cols"), A_FLOAT, 0);

        /* inicia el comportament de videogrid */

        videogrid_setwidget();

        class_setwidget(videogrid_class, &videogrid_widgetbehavior);
        class_sethelpsymbol(videogrid_class, gensym("videogrid.pd"));
        class_setsavefn(videogrid_class, &videogrid_save);
        class_setpropertiesfn(videogrid_class, videogrid_properties);
    }
}
