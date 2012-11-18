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

#ifndef _FILE_INPUT__
#define _FILE_INPUT__

#define USE_LIBSNDFILE      0
#define USE_LIBVORBISFILE   1

//! returns 1 if s is in [0,c)
#define CHECK_SEEK(s, c) (s<0 ? 0 : (s>=c ? 0 : 1))

/*!
 * checks which library to use
 * 
 * @param file filename
 * @return USE_LIBSNDFILE or USE_LIBVORBISFILE, -1 if there was an error
 */
int check_fileformat(t_symbol* file);

/*!
 * read audio data with libsndfile
 * 
 * @param helper_arrays (unallocated) pointer to the data
 * @param channel_count nr of channels
 * @param seek frames to seek in file
 * @param resize 1 if array should be resized
 * @param array_size size of the array in samples
 * @param file filename
 * @return new arraysiize, -1 if there was a failure
 */
int read_libsndfile(t_float** helper_arrays, int channel_count, int seek,
                  int resize, int array_size, t_symbol* file);

/*!
 * read audio data with libvorbisfile
 * 
 * @param helper_arrays (unallocated) pointer to the data
 * @param channel_count nr of channels
 * @param seek frames to seek in file
 * @param resize 1 if array should be resized
 * @param array_size size of the array in samples
 * @param file filename
 * @return new arraysiize, -1 if there was a failure
 */
int read_libvorbisfile(t_float** helper_arrays, int channel_count, int seek,
                       int resize, int array_size, t_symbol* file);

#endif //_FILE_INPUT__
