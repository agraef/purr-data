/*
 * threaded soundfiler for PD
 * Copyright (C) 2005, Georg Holzmann <grh@mur.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "sndfiler.h"
#include "file_input.h"

int check_fileformat(t_symbol* file)
{
    FILE *fp = NULL;
    OggVorbis_File vorbisfile;

    // partially opens a vorbis file to test for Vorbis-ness
    if( !(fp = fopen(file->s_name, "r")) )
        return -1;

    if( ov_test(fp, &vorbisfile, NULL, 0) < 0 )
    {
        fclose(fp);
        return USE_LIBSNDFILE;
    }

    ov_clear(&vorbisfile);
    return USE_LIBVORBISFILE;
}

int read_libsndfile(t_float** helper_arrays, int channel_count, int seek,
                    int resize, int array_size, t_symbol* file)
{
    int arraysize = array_size;
    int writesize = 0, i=0, j=0;
    SNDFILE* sndfile;
    SF_INFO info;

    sndfile = sf_open(file->s_name, SFM_READ, &info);

    if(!sndfile)
        return -1;

    int pos = 0;
    int maxchannels = (channel_count < info.channels) ?
        channel_count : info.channels;

    t_float * item = alloca(maxchannels * sizeof(t_float));

    // negative seek: offset from the end of the file
    if(seek<0)
    {
        if(CHECK_SEEK(seek+info.frames, info.frames))
            pos = sf_seek(sndfile, seek, SEEK_END);
        else pos = -1;
    }
    if(seek>0)
    {
        if(CHECK_SEEK(seek, info.frames))
            pos = sf_seek(sndfile, seek, SEEK_SET);
        else pos = -1;
    }
    if(pos == -1)
    {
        sf_close(sndfile);
        post("invalid seek in soundfile");
        return -1;
    }
	
    if(resize)
    {
        writesize = (info.frames-pos);
        arraysize = writesize;
    }
    else
        writesize = (arraysize>(info.frames-pos)) ? 
            info.frames-pos : arraysize;

#if (_POSIX_MEMLOCK - 0) >=  200112L
    munlockall();
#endif

    for (i = 0; i != channel_count; ++i)
    {
        helper_arrays[i] = getalignedbytes(arraysize * sizeof(t_float));
    }

    for (i = 0; i != writesize; ++i)
    {
        sf_read_float(sndfile, item, info.channels);

        for (j = 0; j != info.channels; ++j)
        {
            if (j < channel_count)
            {
                helper_arrays[j][i] = item[j];
            }
        }
    }

    // fill remaining elements with zero
    if(!resize && (arraysize > (info.frames-pos)))
    {
        for (i = writesize; i != arraysize; ++i)
        {
            for (j = 0; j != info.channels; ++j)
            {
                if (j < channel_count)
                {
                    helper_arrays[j][i] = 0;
                }
            }
        }
    }

#if (_POSIX_MEMLOCK - 0) >=  200112L
    mlockall(MCL_FUTURE);
#endif

    sf_close(sndfile);
    return arraysize;
}

int read_libvorbisfile(t_float** helper_arrays, int channel_count, int seek,
                       int resize, int array_size, t_symbol* file)
{
    int arraysize = array_size;
    int writesize = 0, i=0, j=0;
    int pos=0, maxchannels=0, frames=0, frames_read=0;
    int current_section=0, finished=0;
    float **buftmp = NULL;
    FILE *fp = NULL;
    OggVorbis_File vorbisfile;
    vorbis_info *info;

    if( !(fp = fopen(file->s_name, "r")) )
        return -1;

    if( ov_open(fp, &vorbisfile, NULL, 0) < 0 )
    {
        fclose(fp);
        return -1;
    }

    info = ov_info(&vorbisfile, -1);
    frames = ov_pcm_total(&vorbisfile, -1);
    if( !info || frames==OV_EINVAL )
    {
        ov_clear(&vorbisfile);
        post("failed to get info about vorbis file");
        return -1;
    }

    maxchannels = (channel_count < info->channels) ?
                   channel_count : info->channels;

    // negative seek: offset from the end of the file
    if(seek<0)
    {
        if(CHECK_SEEK(frames+seek, frames))
        {
            int ret = ov_pcm_seek(&vorbisfile, frames+seek);
            if(ret!=0) 
                pos =-1;
            else
                pos = frames+seek;
        }
        else pos = -1;
    }
    if(seek>0)
    {
        if(CHECK_SEEK(seek, frames))
        {
            int ret = ov_pcm_seek(&vorbisfile, seek);
            if(ret!=0)
                pos =-1;
            else
                pos = seek;
        }
        else pos = -1;
    }
    if(pos == -1)
    {
        ov_clear(&vorbisfile);
        post("invalid seek in vorbis file");
        return -1;
    }

    if(resize)
    {
        writesize = (frames-pos);
        arraysize = writesize;
    }
    else
        writesize = (arraysize>(frames-pos)) ? 
                     frames-pos : arraysize;

#if (_POSIX_MEMLOCK - 0) >=  200112L
    munlockall();
#endif

    for (i = 0; i != channel_count; ++i)
    {
        helper_arrays[i] = getalignedbytes(arraysize * sizeof(t_float));
    }

    for (i = 0; i != writesize; ++i)
    {
        int ret = ov_read_float(&vorbisfile, &buftmp, 1, 
                                &current_section);
        if(ret!=1)
            post("wrong return type while ogg decoding!");

        for (j = 0; j != channel_count; ++j)
        {
            helper_arrays[j][i] = buftmp[j][0];
        }
    }

    // fill remaining elements with zero
    if(!resize && (arraysize > (frames-pos)))
    {
        for (i = writesize; i != arraysize; ++i)
        {
            for (j = 0; j != info->channels; ++j)
            {
                if (j < channel_count)
                {
                    helper_arrays[j][i] = 0;
                }
            }
        }
    }

#if (_POSIX_MEMLOCK - 0) >=  200112L
    mlockall(MCL_FUTURE);
#endif

    ov_clear(&vorbisfile);
    return arraysize;
}
