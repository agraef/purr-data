/* pluginhost~ - A plugin host for Pd
 *
 * Copyright (C) 2012 Jamie Bullock and others
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <sys/wait.h>
#include <stdlib.h>

#include "ph_common.h"
#include "handlers_pd.h"

static t_class *ph_class;

static void *ph_new(t_symbol *s, t_int argc, t_atom *argv)
{

    ph *x = (ph *)pd_new(ph_class);
    post("\n========================================\n"
            "%s(): version %.2f\n========================================\n", 
            PH_NAME, PH_VERSION);

    ph_init_plugin(x);

    x->sr       = (int)sys_getsr();
    x->sr_inv   = 1 / (t_float)x->sr;
    x->dsp      = 0;
    x->time_ref = (t_int)clock_getlogicaltime;
    x->blksize  = sys_getblksize();
    x->x_canvas = canvas_getcurrent();

    return ph_load_plugin(x, argc, argv);

}

static void ph_free(ph *x)
{
    ph_quit_plugin(x);
    ph_free_plugin(x);
}

static void ph_sigchld_handler(int sig)
{
    wait(NULL);
}

void pluginhost_tilde_setup(void)
{

    ph_class = class_new(gensym("pluginhost~"), (t_newmethod)ph_new,
            (t_method)ph_free, sizeof(ph), 0, A_GIMME, 0);
    class_addlist(ph_class, handle_pd_list);
    class_addbang(ph_class, handle_pd_bang);
    class_addmethod(ph_class, (t_method)handle_pd_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod (ph_class,(t_method)handle_pd_info, gensym ("info"), 0);
    class_addmethod(ph_class, (t_method)handle_pd_dssi, 
            gensym("dssi"), A_GIMME, 0);
    class_addmethod (ph_class,(t_method)handle_pd_control, 
            gensym ("control"),A_DEFSYM, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod (ph_class,(t_method)handle_pd_listplugins,
            gensym ("listplugins"),0);
    class_addmethod (ph_class,(t_method)handle_pd_reset,
            gensym ("reset"), A_DEFFLOAT, 0);
    class_addmethod (ph_class, (t_method)handle_pd_osc,
            gensym("osc"), A_GIMME, 0);
    class_sethelpsymbol(ph_class, gensym("pluginhost~-help"));

    CLASS_MAINSIGNALIN(ph_class, ph, f);
    signal(SIGCHLD, ph_sigchld_handler);
}

