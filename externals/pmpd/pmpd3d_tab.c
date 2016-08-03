#define car2sphR(z,x,y) (sqrt(x*x + y*y + z*z))
#define car2sphTeta(z,x,y,r) (acos(z/r))
#define car2sphPhy(z,x,y) (atan2(y,x))

void pmpd3d_massesPosT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize/3 );
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = x->mass[i].posX;
                vec[3*i+1].w_float = x->mass[i].posY;
                vec[3*i+2].w_float = x->mass[i].posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].posX;
                    i++;
                    vec[i].w_float = x->mass[j].posY;
                    i++;
                    vec[i].w_float = x->mass[j].posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesPosSphericalT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    t_float R;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) ) // table name
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize/3 );
            for (i=0; i < taille_max ; i++)
            {   
                R = car2sphR(x->mass[i].posX, x->mass[i].posY, x->mass[i].posZ);
                vec[3*i  ].w_float = R;
                vec[3*i+1].w_float = car2sphTeta(x->mass[i].posX, x->mass[i].posY, x->mass[i].posZ, R);
                vec[3*i+2].w_float = car2sphPhy(x->mass[i].posX, x->mass[i].posY, x->mass[i].posZ);
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) ) // mass Id; table name; 
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    R =  car2sphR(x->mass[j].posX, x->mass[j].posY, x->mass[j].posZ);
                    vec[i].w_float = R;
                    i++;
                    vec[i].w_float = car2sphTeta(x->mass[j].posX, x->mass[j].posY, x->mass[j].posZ, R);
                    i++;
                    vec[i].w_float = car2sphPhy(x->mass[j].posX, x->mass[j].posY, x->mass[j].posZ);
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesSpeedsT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize/3);
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = x->mass[i].speedX;
                vec[3*i+1].w_float = x->mass[i].speedY;
                vec[3*i+2].w_float = x->mass[i].speedZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].speedX;
                    i++;
                    vec[i].w_float = x->mass[j].speedY;
                    i++;
                    vec[i].w_float = x->mass[j].speedZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesForcesT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize/3);
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = x->mass[i].forceX;
                vec[3*i+1].w_float = x->mass[i].forceY;
                vec[3*i+2].w_float = x->mass[i].forceZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].forceX;
                    i++;
                    vec[i].w_float = x->mass[j].forceY;
                    i++;
                    vec[i].w_float = x->mass[j].forceZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesPosXT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].posX;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)

                {
                    vec[i].w_float = x->mass[j].posX;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesSpeedsXT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {        
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].speedX;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].speedX;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesForcesXT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].forceX;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].forceX;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesPosYT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].posY;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].posY;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesSpeedsYT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].speedY;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].speedY;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesForcesYT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].forceY;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].forceY;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesPosZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesSpeedsZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].speedZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].speedZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesForcesZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->mass[i].forceZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = x->mass[j].forceZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

// ---------------------------------------------------------------------

void pmpd3d_massesPosNormT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = sqrt(sqr(x->mass[i].posX)+sqr(x->mass[i].posY)+sqr(x->mass[i].posZ));
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = sqrt(sqr(x->mass[j].posX)+sqr(x->mass[j].posY)+sqr(x->mass[i].posZ));
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesSpeedsNormT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = sqrt(sqr(x->mass[i].speedX)+sqr(x->mass[i].speedY)+sqr(x->mass[i].speedZ));
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = sqrt(sqr(x->mass[j].speedX)+sqr(x->mass[j].speedY)+sqr(x->mass[i].speedZ));
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_massesForcesNormT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_mass;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = sqrt(sqr(x->mass[i].forceX)+sqr(x->mass[i].forceY)+sqr(x->mass[i].forceZ));
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_mass))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
                {
                    vec[i].w_float = sqrt(sqr(x->mass[j].forceX)+sqr(x->mass[j].forceY)+sqr(x->mass[i].forceZ));
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

// ---------------------------------------------------------------------

void pmpd3d_linkPosT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/3);
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = (x->link[i].mass2->posX + x->link[i].mass1->posX)/2;
                vec[3*i+1].w_float = (x->link[i].mass2->posY + x->link[i].mass1->posY)/2;
                vec[3*i+2].w_float = (x->link[i].mass2->posZ + x->link[i].mass1->posZ)/2;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = (x->link[j].mass2->posX + x->link[j].mass1->posX)/2;
                    i++;
                    vec[i].w_float = (x->link[j].mass2->posY + x->link[j].mass1->posY)/2;
                    i++;
                    vec[i].w_float = (x->link[j].mass2->posZ + x->link[j].mass1->posZ)/2;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/3);
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = x->link[i].mass2->posX - x->link[i].mass1->posX;
                vec[3*i+1].w_float = x->link[i].mass2->posY - x->link[i].mass1->posY;
                vec[3*i+2].w_float = x->link[i].mass2->posZ - x->link[i].mass1->posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->posX + x->link[j].mass1->posX;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posY + x->link[j].mass1->posY;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posZ + x->link[j].mass1->posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkPosSpeedT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/3);
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = (x->link[i].mass2->speedX + x->link[i].mass1->speedX)/2;
                vec[3*i+1].w_float = (x->link[i].mass2->speedY + x->link[i].mass1->speedY)/2;
                vec[3*i+2].w_float = (x->link[i].mass2->speedZ + x->link[i].mass1->speedZ)/2;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = (x->link[j].mass2->speedX + x->link[j].mass1->speedX)/2;
                    i++;
                    vec[i].w_float = (x->link[j].mass2->speedY + x->link[j].mass1->speedY)/2;
                    i++;
                    vec[i].w_float = (x->link[j].mass2->speedZ + x->link[j].mass1->speedZ)/2;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthSpeedT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/3);
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = x->link[i].mass2->speedX - x->link[i].mass1->speedX;
                vec[3*i+1].w_float = x->link[i].mass2->speedY - x->link[i].mass1->speedY;
                vec[3*i+2].w_float = x->link[i].mass2->speedZ - x->link[i].mass1->speedZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->speedX + x->link[j].mass1->speedX;
                    i++;
                    vec[i].w_float = x->link[j].mass2->speedY + x->link[j].mass1->speedY;
                    i++;
                    vec[i].w_float = x->link[j].mass2->speedZ + x->link[j].mass1->speedZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkPosXT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = (x->link[i].mass1->posX + x->link[i].mass2->posX)/2;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = (x->link[j].mass1->posX + x->link[j].mass2->posX)/2;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthXT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->posX - x->link[i].mass1->posX;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->posX - x->link[j].mass1->posX;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkPosSpeedXT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = (x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = (x->link[j].mass1->speedX + x->link[j].mass2->speedX)/2;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthSpeedXT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->speedX - x->link[i].mass1->speedX;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->speedX - x->link[j].mass1->speedX;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkPosYT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = (x->link[i].mass1->posY + x->link[i].mass2->posY)/2;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = (x->link[j].mass1->posY + x->link[j].mass2->posY)/2;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthYT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->posY - x->link[i].mass1->posY;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->posY - x->link[j].mass1->posY;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkPosSpeedYT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = (x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = (x->link[j].mass1->speedY + x->link[j].mass2->speedY)/2;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthSpeedYT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->speedY - x->link[i].mass1->speedY;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->speedY - x->link[j].mass1->speedY;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkPosZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = (x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = (x->link[j].mass1->posZ + x->link[j].mass2->posZ)/2;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->posZ - x->link[i].mass1->posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->posZ - x->link[j].mass1->posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkPosSpeedZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = (x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = (x->link[j].mass1->speedZ + x->link[j].mass2->speedZ)/2;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthSpeedZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->speedZ - x->link[i].mass1->speedZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->speedZ - x->link[j].mass1->speedZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

// ---------------------------------------------------------------------

void pmpd3d_linkPosNormT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = sqrt( \
                            sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + \
                            sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + \
                            sqr((x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2) );
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[j].w_float = sqrt( \
                            sqr((x->link[j].mass1->posX + x->link[j].mass2->posX)/2) + \
                            sqr((x->link[j].mass1->posY + x->link[j].mass2->posY)/2) + \
                            sqr((x->link[j].mass1->posZ + x->link[j].mass2->posZ)/2) );
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthNormT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = sqrt( \
                            sqr(x->link[i].mass2->posX - x->link[i].mass1->posX) + \
                            sqr(x->link[i].mass2->posY - x->link[i].mass1->posY) + \
                            sqr(x->link[i].mass2->posZ - x->link[i].mass1->posZ) );
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = sqrt( \
                            sqr(x->link[j].mass2->posX - x->link[j].mass1->posX) + \
                            sqr(x->link[j].mass2->posY - x->link[j].mass1->posY) + \
                            sqr(x->link[j].mass2->posZ - x->link[j].mass1->posZ) );
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkPosSpeedNormT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = sqrt( \
                            sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2) + \
                            sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + \
                            sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2) );
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = sqrt( \
                            sqr((x->link[j].mass1->speedX + x->link[j].mass2->speedX)/2) + \
                            sqr((x->link[j].mass1->speedY + x->link[j].mass2->speedY)/2) + \
                            sqr((x->link[j].mass1->speedZ + x->link[j].mass2->speedZ)/2) );
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkLengthSpeedNormT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = sqrt( \
                            sqr(x->link[i].mass2->speedX - x->link[i].mass1->speedX) + \
                            sqr(x->link[i].mass2->speedY - x->link[i].mass1->speedY) + \
                            sqr(x->link[i].mass2->speedZ - x->link[i].mass1->speedZ) );
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = sqrt( \
                            sqr(x->link[j].mass2->speedX - x->link[j].mass1->speedX) + \
                            sqr(x->link[j].mass2->speedY - x->link[j].mass1->speedY) + \
                            sqr(x->link[j].mass2->speedZ - x->link[j].mass1->speedZ) );
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

// ---------------------------------------------------------------------

void pmpd3d_linkEndT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/6  );
            for (i=0; i < taille_max ; i++)
            {
                vec[6*i  ].w_float = x->link[i].mass1->posX;
                vec[6*i+1].w_float = x->link[i].mass1->posY;
                vec[6*i+2].w_float = x->link[i].mass1->posZ;
                vec[6*i+3].w_float = x->link[i].mass2->posX;
                vec[6*i+4].w_float = x->link[i].mass2->posY;
                vec[6*i+5].w_float = x->link[i].mass2->posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-5) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass1->posX;
                    i++;
                    vec[i].w_float = x->link[j].mass1->posY;
                    i++;
                    vec[i].w_float = x->link[j].mass1->posZ;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posX;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posY;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEndXT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/2 );
            for (i=0; i < taille_max ; i++)
            {
                vec[2*i  ].w_float = x->link[i].mass1->posX;
                vec[2*i+1].w_float = x->link[i].mass2->posX;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-1) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass1->posX;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posX;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEndYT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/2);
            for (i=0; i < taille_max ; i++)
            {
                vec[2*i  ].w_float = x->link[i].mass1->posY;
                vec[2*i+1].w_float = x->link[i].mass2->posY;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-1) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass1->posY;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posY;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEndZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/2);
            for (i=0; i < taille_max ; i++)
            {
                vec[2*i  ].w_float = x->link[i].mass1->posZ;
                vec[2*i+1].w_float = x->link[i].mass2->posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-1) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass1->posZ;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEnd1T(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/3);
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = x->link[i].mass1->posX;
                vec[3*i+1].w_float = x->link[i].mass1->posY;
                vec[3*i+2].w_float = x->link[i].mass1->posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass1->posX;
                    i++;
                    vec[i].w_float = x->link[j].mass1->posY;
                    i++;
                    vec[i].w_float = x->link[j].mass1->posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEnd1XT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass1->posX;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass1->posX;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEnd1YT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass1->posY;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass1->posY;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEnd1ZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass1->posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass1->posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEnd2T(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize/3);
            for (i=0; i < taille_max ; i++)
            {
                vec[3*i  ].w_float = x->link[i].mass2->posX;
                vec[3*i+1].w_float = x->link[i].mass2->posY;
                vec[3*i+2].w_float = x->link[i].mass2->posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize-2) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->posX;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posY;
                    i++;
                    vec[i].w_float = x->link[j].mass2->posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEnd2XT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->posX;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->posX;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEnd2YT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->posY;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->posY;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}

void pmpd3d_linkEnd2ZT(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j, vecsize;
    t_garray *a;
    t_word *vec;
    
    if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {
            int taille_max = x->nb_link;
            taille_max = min(taille_max, vecsize);
            for (i=0; i < taille_max ; i++)
            {
                vec[i].w_float = x->link[i].mass2->posZ;
            }
            garray_redraw(a);
        }
    }
    else 
    if ( (argc==2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
    {
        t_symbol *tab_name = atom_getsymbolarg(0, argc, argv);
        if (!(a = (t_garray *)pd_findbyclass(tab_name, garray_class)))
            pd_error(x, "%s: no such array", tab_name->s_name);
        else if (!garray_getfloatwords(a, &vecsize, &vec))
            pd_error(x, "%s: bad template for tabwrite", tab_name->s_name);
        else
        {    
            i = 0;
            j = 0;
            while ((i < vecsize) && (j < x->nb_link))
            {
                if (atom_getsymbolarg(1,argc,argv) == x->link[j].Id)
                {
                    vec[i].w_float = x->link[j].mass2->posZ;
                    i++;
                }
                j++;
            }
            garray_redraw(a);
        }
    }
}
