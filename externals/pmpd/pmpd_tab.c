void pmpd_massesPosT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd_massesSpeedsT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd_massesForcesT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd_linksPosT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd_linksLengthT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd_linksPosSpeedT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd_linksLengthSpeedT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd_linkEndT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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
            while ((i < vecsize) && (j < x->nb_link))
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

void pmpd_linkEnd1T(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd_linkEnd2T(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
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
