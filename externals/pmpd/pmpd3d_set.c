#define sph2carX(r,phy,teta) (r * cos(teta))
#define sph2carY(r,phy,teta) (r * sin(teta) * cos(phy))
#define sph2carZ(r,phy,teta) (r * sin(teta) * sin(phy))

void pmpd3d_setK(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;
    
    if ( (argc==2) &&( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].K = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].K = atom_getfloatarg(1, argc, argv);
            }
        }
    }
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_link; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
				{
					x->link[i].K = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_link; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
				{
					x->link[i].K = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min(npoints,x->nb_link-atom_getfloatarg(1, argc, argv));
			for (i=0; i < n; i++)
			{
					x->link[i+offset].K = K*vec[i].w_float;
			}
		}
	}
}

void pmpd3d_setD(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

    if ( (argc==2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].D = atom_getfloatarg(1, argc, argv);
    }
    if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].D = atom_getfloatarg(1, argc, argv);
            }
        }
    }
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_link; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
				{
					x->link[i].D = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_link; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
				{
					x->link[i].D = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min(npoints,x->nb_link-atom_getfloatarg(1, argc, argv));
			for (i=0; i < n; i++)
			{
					x->link[i+offset].D = K*vec[i].w_float;
			}
		}
	}
}

void pmpd3d_setPow(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;
    
    if ( (argc==2) &&( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].Pow = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].Pow = atom_getfloatarg(1, argc, argv);
            }
        }
    }
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_link; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
				{
					x->link[i].Pow = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_link; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
				{
					x->link[i].Pow = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min(npoints,x->nb_link-atom_getfloatarg(1, argc, argv));
			for (i=0; i < n; i++)
			{
					x->link[i+offset].K = K*vec[i].w_float;
			}
		}
	}
}

void pmpd3d_setDEnv(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].D2 = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].D2 = atom_getfloatarg(1, argc, argv);
            }
        }
    }
    else if ( (argc == 1) && ( argv[0].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            x->mass[i].D2 = atom_getfloatarg(0, argc, argv);
        }
    }
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].D2 = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].D2 = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min(npoints,x->nb_mass-atom_getfloatarg(1, argc, argv));
			for (i=0; i < n; i++)
			{
					x->mass[i+offset].D2 = K*vec[i].w_float;
			}
		}
	}
}

void pmpd3d_setDEnvOffset(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].D2offset = atom_getfloatarg(0, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].D2offset = atom_getfloatarg(1, argc, argv);
            }
        }
    }
    else if ( (argc == 1) && ( argv[0].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            x->mass[i].D2offset = atom_getfloatarg(0, argc, argv);
        }
    }
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].D2offset = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].D2offset = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min(npoints,x->nb_mass-atom_getfloatarg(1, argc, argv));
			for (i=0; i < n; i++)
			{
					x->mass[i+offset].D2offset = K*vec[i].w_float;
			}
		}
	}
}

void pmpd3d_setL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    { // set a link to a specific size
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].L = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    { // set a class of link to a specific size
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].L = atom_getfloatarg(1, argc, argv);
            }
        }
    }
    else if ( (argc == 1) && ( argv[0].a_type == A_FLOAT ) )
    { // set a link to it's curent size
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].L = x->link[tmp].distance;
	}
    else if ( (argc == 1) && ( argv[0].a_type == A_SYMBOL ) )
    { // set a class of link to there curent size
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].L = x->link[i].distance;            
		     }
        }
    }
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_link; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
				{
					x->link[i].L = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
}

void pmpd3d_addL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    { // set a link to a specific size
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].L += atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    { // set a class of link to a specific size
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].L += atom_getfloatarg(1, argc, argv);
            }
        }
    }
    else if ( (argc == 1) && ( argv[0].a_type == A_FLOAT ) )
    { // set a link to it's curent size
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].L += x->link[tmp].distance;
	}
    else if ( (argc == 1) && ( argv[0].a_type == A_SYMBOL ) )
    { // set a class of link to there curent size
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].L += x->link[i].distance;            
		     }
        }
    }
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_link; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
				{
					x->link[i].L += K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
}

void pmpd3d_setLCurrent(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( ( argc == 1 ) && ( argv[0].a_type == A_FLOAT ) )
    { // set a link to it's current length
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].L = x->link[tmp].distance;
    }
    else if ( ( argc == 1 ) && ( argv[0].a_type == A_SYMBOL ) )
    { // set a class of link to there current length
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].L = x->link[i].distance;
            }
        }
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    { // set a link to a mix between it's curent length and it's size
        i = atom_getfloatarg(0, argc, argv);
        i = max(0, min( x->nb_link-1, i));
        x->link[i].L = mix(x->link[i].L,x->link[i].distance,atom_getfloatarg(1, argc, argv));
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    { // set a class of link to a mix between it's curent length and it's size
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].L = mix(x->link[i].L,x->link[i].distance,atom_getfloatarg(1, argc, argv));
            }
        }
    }
}

void pmpd3d_setLKTab(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;
    t_float K_l = atom_getfloatarg(1, argc, argv);
    if (K_l <=  0) K_l = 1;
    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].K_L = K_l;
    }
    if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].K_L = K_l;
            }
        }
    }
}

void pmpd3d_setLDTab(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;
    t_float D_l = atom_getfloatarg(1, argc, argv);
    if (D_l <=  0) D_l = 1;
    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].D_L = D_l;
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].D_L = D_l;
            }
        }
    }
}

void pmpd3d_setLinkId(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].Id = atom_getsymbolarg(1, argc, argv);
    }
    else if ( ((argc == 2) &&  argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].Id = atom_getsymbolarg(1, argc, argv);
            }
        }
    }
}

void pmpd3d_setMassId(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].Id = atom_getsymbolarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].Id = atom_getsymbolarg(1, argc, argv);
            }
        }
    }
}

void pmpd3d_setFixed(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 1) && (argv[0].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].mobile = 0;
    }
    else if ( (argc == 1) && (argv[0].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].mobile = 0;
            }
        }
    }
}

void pmpd3d_setMobile(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 1) && ( argv[0].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].mobile = 1;
    }
    else if ( (argc == 1) && ( argv[0].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].mobile = 1;
            }
        }
    }
}

void pmpd3d_setSpeed(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 4) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) && ( argv[3].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].speedX = atom_getfloatarg(1, argc, argv);
        x->mass[tmp].speedY = atom_getfloatarg(2, argc, argv);
        x->mass[tmp].speedZ = atom_getfloatarg(3, argc, argv);
    }
    else if ( (argc == 4) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) && ( argv[3].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].speedX = atom_getfloatarg(1, argc, argv);
                x->mass[i].speedY = atom_getfloatarg(2, argc, argv);
                x->mass[i].speedZ = atom_getfloatarg(3, argc, argv);
            }
        }
    }
}

void pmpd3d_setSpeedX(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].speedX = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].speedX = atom_getfloatarg(1, argc, argv);
            }
        }
    }
}

void pmpd3d_setSpeedY(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].speedY = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].speedY = atom_getfloatarg(1, argc, argv);
            }
        }
    }
}

void pmpd3d_setSpeedZ(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].speedZ = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].speedZ = atom_getfloatarg(1, argc, argv);
            }
        }
    }
}

void pmpd3d_setForce(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 4) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) && ( argv[3].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].forceX = atom_getfloatarg(1, argc, argv);
        x->mass[tmp].forceY = atom_getfloatarg(2, argc, argv);
        x->mass[tmp].forceZ = atom_getfloatarg(3, argc, argv);
    }
    else if ( (argc == 4) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) && ( argv[3].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].forceX = atom_getfloatarg(1, argc, argv);
                x->mass[i].forceY = atom_getfloatarg(2, argc, argv);
                x->mass[i].forceZ = atom_getfloatarg(3, argc, argv);
            }
        }
    }
}

void pmpd3d_setForceX(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].forceX = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].forceX = atom_getfloatarg(1, argc, argv);
            }
        }
    }
}

void pmpd3d_setForceY(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].forceY = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].forceY = atom_getfloatarg(1, argc, argv);
            }
        }
    }
}

void pmpd3d_setForceZ(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].forceZ = atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].forceZ = atom_getfloatarg(1, argc, argv);
            }
        }
    }
}
		
void pmpd3d_setActivei(t_pmpd3d *x, int i)
{
	float Lx, Ly,Lz, L;
	Lx = x->link[i].mass1->posX - x->link[i].mass2->posX;
	Ly = x->link[i].mass1->posY - x->link[i].mass2->posY;
	Lz = x->link[i].mass1->posZ - x->link[i].mass2->posZ;
	L = sqrt( sqr(Lx) + sqr(Ly) + sqr(Lz) );
	x->link[i].distance = L; 
	x->link[i].active = 1;
}

void pmpd3d_setActive(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 1) && ( argv[0].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
		pmpd3d_setActivei(x,tmp);
    }
    else if ( (argc == 1) && ( argv[0].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id )
            {
				pmpd3d_setActivei(x,i);
            }
        }
    }
    else if ( argc == 0 ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
			pmpd3d_setActivei(x,i);
        }
    }
}

void pmpd3d_setInactive(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 1) && ( argv[0].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
        x->link[tmp].active = 0;
    }
    else if ( (argc == 1) && ( argv[0].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
                x->link[i].active = 0;
            }
        }
    }
    else if ( argc == 0 ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
			x->link[i].active = 0;
        }
    }
}

void pmpd3d_pos(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
// displace a mass to a certain position
	t_int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

	if ( (argc == 4) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) && ( argv[3].a_type == A_FLOAT ) )
	{
		tmp = atom_getfloatarg(0, argc, argv);
		tmp = max(0, min( x->nb_mass-1, tmp));
		x->mass[tmp].posX = atom_getfloatarg(1, argc, argv);
		   x->mass[tmp].speedX = 0;
		x->mass[tmp].forceX = 0;
		   x->mass[tmp].posY = atom_getfloatarg(2, argc, argv);
		   x->mass[tmp].speedY = 0;
		x->mass[tmp].forceY = 0;
		   x->mass[tmp].posZ = atom_getfloatarg(3, argc, argv);
		   x->mass[tmp].speedZ = 0;
		x->mass[tmp].forceZ = 0;
	}
	else if ( (argc == 4) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) && ( argv[3].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{
				x->mass[i].posX = atom_getfloatarg(1, argc, argv);
				x->mass[i].speedX = 0;
				x->mass[i].forceX = 0;
				x->mass[i].posY = atom_getfloatarg(2, argc, argv);
				x->mass[i].speedY = 0;
				x->mass[i].forceY = 0;
				x->mass[i].posZ = atom_getfloatarg(3, argc, argv);
				x->mass[i].speedZ = 0;
				x->mass[i].forceZ = 0;
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].posX = K*vec[n].w_float;
                    x->mass[i].speedX = 0; 
                    x->mass[i].forceX = 0;
					n++;
					x->mass[i].posY = K*vec[n].w_float;
                    x->mass[i].speedY = 0; 
                    x->mass[i].forceY = 0;
					n++;
					x->mass[i].posZ = K*vec[n].w_float;
                    x->mass[i].speedZ = 0; 
                    x->mass[i].forceZ = 0;
					n++;
					if (n >= npoints +2) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min((int)npoints/2,x->nb_mass-offset);
			for (i=0; i < n; i++)
			{
					x->mass[i+offset].posX = K*vec[3*i].w_float;
                    x->mass[i+offset].speedX = 0; 
                    x->mass[i+offset].forceX = 0;
					x->mass[i+offset].posY = K*vec[3*i+1].w_float;
                    x->mass[i+offset].speedY = 0; 
                    x->mass[i+offset].forceY = 0; 
					x->mass[i+offset].posZ = K*vec[3*i+1].w_float;
                    x->mass[i+offset].speedZ = 0; 
                    x->mass[i+offset].forceZ = 0;                    
			}
		}
	} 
}

void pmpd3d_posSpherical(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
// displace a mass to a certain position, using polar coordinat system
	t_int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;
    t_float r, phy, teta, X,Y,Z;

	if ( (argc == 4) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) && ( argv[3].a_type == A_FLOAT ) )
	{
		tmp = atom_getfloatarg(0, argc, argv);
		tmp = max(0, min( x->nb_mass-1, tmp));
        r = atom_getfloatarg(1, argc, argv);
        teta = atom_getfloatarg(2, argc, argv);
        phy = atom_getfloatarg(3, argc, argv);
		x->mass[tmp].posX = sph2carX(r,phy,teta);
        x->mass[tmp].speedX = 0;
		x->mass[tmp].forceX = 0;
        x->mass[tmp].posY = sph2carY(r,phy,teta);
        x->mass[tmp].speedY = 0;
		x->mass[tmp].forceY = 0;
        x->mass[tmp].posZ = sph2carZ(r,phy,teta);
        x->mass[tmp].speedZ = 0;
		x->mass[tmp].forceZ = 0;
	}
	else if ( (argc == 4) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) && ( argv[3].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{
                r = atom_getfloatarg(1, argc, argv);
                teta = atom_getfloatarg(2, argc, argv);
                phy = atom_getfloatarg(3, argc, argv);
				x->mass[i].posX = sph2carX(r,phy,teta);
				x->mass[i].speedX = 0;
				x->mass[i].forceX = 0;
				x->mass[i].posY = sph2carY(r,phy,teta);
				x->mass[i].speedY = 0;
				x->mass[i].forceY = 0;
				x->mass[i].posZ = sph2carZ(r,phy,teta);
				x->mass[i].speedZ = 0;
				x->mass[i].forceZ = 0;
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
                    r = K*vec[n].w_float;
                    n++;
                    teta = K*vec[n].w_float;
                    n++;
                    phy = K*vec[n].w_float;
                    n++;
                    
					x->mass[i].posX = sph2carX(r,phy,teta);
                    x->mass[i].speedX = 0; 
                    x->mass[i].forceX = 0;
					x->mass[i].posY = sph2carY(r,phy,teta);
                    x->mass[i].speedY = 0; 
                    x->mass[i].forceY = 0;
					x->mass[i].posZ = sph2carZ(r,phy,teta);
                    x->mass[i].speedZ = 0; 
                    x->mass[i].forceZ = 0;
					if (n >= npoints +2) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min((int)npoints/2,x->nb_mass-offset);
			for (i=0; i < n; i++)
			{
                r = K*vec[3*i].w_float;
                teta = K*vec[3*i+1].w_float;
                phy = K*vec[3*i+1].w_float;
                
				x->mass[i+offset].posX = sph2carX(r,phy,teta);
                x->mass[i+offset].speedX = 0; 
                x->mass[i+offset].forceX = 0;
				x->mass[i+offset].posY = sph2carY(r,phy,teta);
                x->mass[i+offset].speedY = 0; 
                x->mass[i+offset].forceY = 0; 
				x->mass[i+offset].posZ = sph2carZ(r,phy,teta);
                x->mass[i+offset].speedZ = 0; 
                x->mass[i+offset].forceZ = 0;                    
			}
		}
	} 
}

void pmpd3d_posX(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
// displace a mass to a certain position
	t_int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

	if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
	{
		tmp = atom_getfloatarg(0, argc, argv);
		tmp = max(0, min( x->nb_mass-1, tmp));
		x->mass[tmp].posX = atom_getfloatarg(1, argc, argv);
		   x->mass[tmp].speedX = 0;
		x->mass[tmp].forceX = 0;

	}
	else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{
				x->mass[i].posX = atom_getfloatarg(1, argc, argv);
				x->mass[i].speedX = 0;
				x->mass[i].forceX = 0;

			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].posX = K*vec[n].w_float;
                    x->mass[i].speedX = 0; 
                    x->mass[i].forceX = 0;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min(npoints,x->nb_mass-offset);
			for (i=0; i < n; i++)
			{
					x->mass[i+offset].posX = K*vec[i].w_float;
                    x->mass[i+offset].speedX = 0; 
                    x->mass[i+offset].forceX = 0;
			}
		}
	} 
}

void pmpd3d_posY(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
// displace a mass to a certain position
	t_int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

	if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
	{
		tmp = atom_getfloatarg(0, argc, argv);
		tmp = max(0, min( x->nb_mass-1, tmp));
		x->mass[tmp].posY = atom_getfloatarg(1, argc, argv);
		   x->mass[tmp].speedY = 0;
		x->mass[tmp].forceY = 0;

	}
	else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{
				x->mass[i].posY = atom_getfloatarg(1, argc, argv);
				x->mass[i].speedY = 0;
				x->mass[i].forceY = 0;
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].posY = K*vec[n].w_float;
                    x->mass[i].speedY = 0; 
                    x->mass[i].forceY = 0;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min(npoints,x->nb_mass-offset);
			for (i=0; i < n; i++)
			{
					x->mass[i+offset].posY = K*vec[i].w_float;
                    x->mass[i+offset].speedY = 0; 
                    x->mass[i+offset].forceY = 0;
			}
		}
	} 
}

void pmpd3d_posZ(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
// displace a mass to a certain position
	t_int tmp, i, offset;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

	if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
	{
		tmp = atom_getfloatarg(0, argc, argv);
		tmp = max(0, min( x->nb_mass-1, tmp));
		x->mass[tmp].posZ = atom_getfloatarg(1, argc, argv);
		   x->mass[tmp].speedZ = 0;
		x->mass[tmp].forceZ = 0;

	}
	else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{
				x->mass[i].posZ = atom_getfloatarg(1, argc, argv);
				x->mass[i].speedZ = 0;
				x->mass[i].forceZ = 0;
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].posZ = K*vec[n].w_float;
                    x->mass[i].speedZ = 0; 
                    x->mass[i].forceZ = 0;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
	else if ( (argc >= 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_SYMBOL ) )
	{
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			offset = atom_getfloatarg(0, argc, argv);
			n=min(npoints,x->nb_mass-offset);
			for (i=0; i < n; i++)
			{
					x->mass[i+offset].posZ = K*vec[i].w_float;
                    x->mass[i+offset].speedZ = 0; 
                    x->mass[i+offset].forceZ = 0;
			}
		}
	} 
}

void pmpd3d_overdamp(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
// set the overdamped factor to a mass
	t_int tmp, i;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;

	if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
	{
		tmp = atom_getfloatarg(0, argc, argv);
		tmp = max(0, min( x->nb_mass-1, tmp));
		x->mass[tmp].overdamp = atom_getfloatarg(1, argc, argv);
	}
	else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{
				x->mass[i].overdamp = atom_getfloatarg(1, argc, argv);
			}
		}
	}
    else if ( (argc >= 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_SYMBOL ) )
    {
		K=1;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )) K=atom_getfloatarg(2, argc, argv);
		if (!(a = (t_garray *)pd_findbyclass(atom_getsymbolarg(1,argc,argv), garray_class)))
			pd_error(x, "%s: no such array", atom_getsymbolarg(1,argc,argv)->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", atom_getsymbolarg(1,argc,argv)->s_name);
		else
		{
			n=0;
			for (i=0; i < x->nb_mass; i++)
			{
				if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
				{
					x->mass[i].overdamp = K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
}

void pmpd3d_setConnection1i(t_pmpd3d *x, int i, int j)
{
	float Lx, Ly,Lz, L;

	x->link[i].mass1=&x->mass[max(0, min( x->nb_mass-1, j))];
	Lx = x->link[i].mass1->posX - x->link[i].mass2->posX;
	Ly = x->link[i].mass1->posY - x->link[i].mass2->posY;
	Lz = x->link[i].mass1->posZ - x->link[i].mass2->posZ;
	L = sqrt( sqr(Lx) + sqr(Ly) + sqr(Lz) );
	x->link[i].distance = L; 
}

void pmpd3d_setEnd1(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	int tmp, i;
	
	if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
		pmpd3d_setConnection1i(x,tmp,atom_getfloatarg(1, argc, argv));

    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id )
            {
				pmpd3d_setConnection1i(x,i,atom_getfloatarg(1, argc, argv));
            }
        }
    }
}

void pmpd3d_setConnection2i(t_pmpd3d *x, int i, int j)
{
	float Lx, Ly,Lz, L;

	x->link[i].mass2=&x->mass[max(0, min( x->nb_mass-1, j))];
	Lx = x->link[i].mass1->posX - x->link[i].mass2->posX;
	Ly = x->link[i].mass1->posY - x->link[i].mass2->posY;
	Lz = x->link[i].mass1->posZ - x->link[i].mass2->posZ;
	L = sqrt( sqr(Lx) + sqr(Ly) + sqr(Lz) );
	x->link[i].distance = L; 
}

void pmpd3d_setEnd2(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	int tmp, i;
	
	if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
		pmpd3d_setConnection2i(x,tmp,atom_getfloatarg(1, argc, argv));

    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id )
            {
				pmpd3d_setConnection2i(x,i,atom_getfloatarg(1, argc, argv));
            }
        }
    }
}

void pmpd3d_setConnectioni(t_pmpd3d *x, int i, int j, int k)
{
	float Lx, Ly,Lz, L;

	x->link[i].mass1=&x->mass[max(0, min( x->nb_mass-1, j))];
	x->link[i].mass2=&x->mass[max(0, min( x->nb_mass-1, k))];

	Lx = x->link[i].mass1->posX - x->link[i].mass2->posX;
	Ly = x->link[i].mass1->posY - x->link[i].mass2->posY;
	Lz = x->link[i].mass1->posZ - x->link[i].mass2->posZ;
	L = sqrt( sqr(Lx) + sqr(Ly) + sqr(Lz) );
	x->link[i].distance = L; 
}

void pmpd3d_setEnd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	int tmp, i;
	
	if ( (argc == 3) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_link-1, tmp));
		pmpd3d_setConnectioni(x,tmp,atom_getfloatarg(1, argc, argv),atom_getfloatarg(2, argc, argv));

    }
    else if ( (argc == 3) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id )
            {
				pmpd3d_setConnectioni(x,i,atom_getfloatarg(1, argc, argv),atom_getfloatarg(2, argc, argv));
            }
        }
    }
}
