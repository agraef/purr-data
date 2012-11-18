/* -----------------------------  clone.h  ------------------------------------ */
/*                                                                              */
/* clone :: abstraction cloner object                                           */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>                                */
/* Get source at http://www.akustische-kunst.org/puredata/clone/                */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/* ---------------------------------------------------------------------------- */

#ifndef __CLONE_H__
#define __CLONE_H__

#define MAXCONNECT 256

extern t_class *clone_in_class;
extern t_class *clone_out_class;

typedef struct _clone_out
{
    t_object   x_obj;
    t_symbol  *x_sym;
	int        x_ab;
} t_clone_out;

typedef struct _clone_sigin
{
    t_object   x_obj;
    t_float   *x_wherefrom;
    int        x_vs;
    t_float    x_f;
} t_clone_sigin;

typedef struct _clone_sigout
{
    t_object   x_obj;
    t_float   *x_whereto;
    int        x_vs;
    t_float    x_f;
} t_clone_sigout;

void clone_in_setup(void);
void clone_sigin_set(t_clone_sigin *x, int vs, t_float *vec);
void clone_sigin_setup(void);
void clone_out_set(t_clone_out *x, int i, t_symbol *s);
void clone_out_setup(void);
void clone_sigout_set(t_clone_sigout *x, int vs, t_float *vec);
void clone_sigout_setup(void);

#endif
