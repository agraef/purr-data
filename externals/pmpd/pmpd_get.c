void pmpd_get(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_symbol *toget; 
    t_atom  toout[3];
    toget = atom_getsymbolarg(0, argc, argv);

    if ( (toget == gensym("massesPos")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->mass[i].posX);
            outlet_anything(x->main_outlet, gensym("massesPosNo"), 2, toout);
        }  
    }
    else
    if ( (toget == gensym("massesPos")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                SETFLOAT(&(toout[0]), i);
                SETFLOAT(&(toout[1]), x->mass[i].posX);
                outlet_anything(x->main_outlet, gensym("massesPosId"), 2, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("massesPos")) && (argc == 1) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->mass[i].posX);
            outlet_anything(x->main_outlet, gensym("massesPos"), 2, toout);
        } 
    }
    else
    if ( (toget == gensym("massesPosName")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), x->mass[i].posX);
            outlet_anything(x->main_outlet, gensym("massesPosNameNo"), 2, toout);
        }  
    }
    else
    if ( (toget == gensym("massesPosName")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                SETSYMBOL(&(toout[0]), x->mass[i].Id);
                SETFLOAT(&(toout[1]), x->mass[i].posX);
                outlet_anything(x->main_outlet, gensym("massesPosNameId"), 2, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("massesPosName")) && (argc == 1) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), x->mass[i].posX);
            outlet_anything(x->main_outlet, gensym("massesPosName"), 2, toout);
        } 
    }
    else
    if ( (toget == gensym("massesSpeeds")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->mass[i].speedX);
            outlet_anything(x->main_outlet, gensym("massesSpeedsNo"), 2, toout);
        }  
    }
    else
    if ( (toget == gensym("massesSpeeds")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                SETFLOAT(&(toout[0]), i);
                SETFLOAT(&(toout[1]), x->mass[i].speedX);
                outlet_anything(x->main_outlet, gensym("massesSpeedsId"), 2, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("massesSpeeds")) && (argc == 1) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->mass[i].speedX);
            outlet_anything(x->main_outlet, gensym("massesSpeeds"), 2, toout);
        } 
    }
    else
    if ( (toget == gensym("massesSpeedsName")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), x->mass[i].speedX);
            outlet_anything(x->main_outlet, gensym("massesSpeedsNameNo"), 2, toout);
        }  
    }
    else
    if ( (toget == gensym("massesSpeedsName")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                SETSYMBOL(&(toout[0]), x->mass[i].Id);
                SETFLOAT(&(toout[1]), x->mass[i].speedX);
                outlet_anything(x->main_outlet, gensym("massesSpeedsNameId"), 2, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("massesSpeedsName")) && (argc == 1) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), x->mass[i].speedX);
            outlet_anything(x->main_outlet, gensym("massesSpeedsName"), 2, toout);
        } 
    }
    else
    if ( (toget == gensym("massesForces")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->mass[i].forceX);
            outlet_anything(x->main_outlet, gensym("massesForcesNo"), 2, toout);
        }  
    }
    else
    if ( (toget == gensym("massesForces")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                SETFLOAT(&(toout[0]), i);
                SETFLOAT(&(toout[1]), x->mass[i].forceX);
                outlet_anything(x->main_outlet, gensym("massesForcesId"), 2, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("massesForces")) && (argc == 1) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->mass[i].forceX);
            outlet_anything(x->main_outlet, gensym("massesForces"), 2, toout);
        } 
    }
    else
    if ( (toget == gensym("massesForcesName")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), x->mass[i].forceX);
            outlet_anything(x->main_outlet, gensym("massesForcesNameNo"), 2, toout);
        }
    }
    else
    if ( (toget == gensym("massesForcesName")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                SETSYMBOL(&(toout[0]), x->mass[i].Id);
                SETFLOAT(&(toout[1]), x->mass[i].forceX);
                outlet_anything(x->main_outlet, gensym("massesForcesNameId"), 2, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("massesForcesName")) && (argc == 1) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), x->mass[i].forceX);
            outlet_anything(x->main_outlet, gensym("massesForcesName"), 2, toout);
        } 
    }
    else
    if ( (toget == gensym("linksPos")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[2]), x->link[i].mass2->posX);
            outlet_anything(x->main_outlet, gensym("linksPosNo"), 3, toout);
        }
    }
    else
    if ( (toget == gensym("linksPos")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->link[i].Id)
            {
                SETFLOAT(&(toout[0]), i);
                SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
                SETFLOAT(&(toout[2]), x->link[i].mass2->posX);
                outlet_anything(x->main_outlet, gensym("linksPosId"), 3, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("linksPos")) && (argc == 1) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[2]), x->link[i].mass2->posX);
            outlet_anything(x->main_outlet, gensym("linksPos"), 3, toout);
        } 
    }
    else
    if ( (toget == gensym("linksPosName")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[2]), x->link[i].mass2->posX);
            outlet_anything(x->main_outlet, gensym("linksPosNameNo"), 3, toout);
        }
    }
    else
    if ( (toget == gensym("linksPosName")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->link[i].Id)
            {
                SETSYMBOL(&(toout[0]), x->link[i].Id);
                SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
                SETFLOAT(&(toout[2]), x->link[i].mass2->posX);
                outlet_anything(x->main_outlet, gensym("linksPosNameId"), 3, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("linksPosName")) && (argc == 1) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[2]), x->link[i].mass2->posX);
            outlet_anything(x->main_outlet, gensym("linksPosName"), 3, toout);
        } 
    }
    else
        error("not get attribute");
}

void pmpd_massPos(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_atom  toout[3];

    if ((argc>0)&&(argv[0].a_type == A_FLOAT))
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].posX);
            outlet_anything(x->main_outlet, gensym("massPos"), 3, toout);
        }  
    }
    else
    if ((argc>0)&&(argv[0].a_type == A_SYMBOL))
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), atom_getsymbolarg(0,argc,argv));
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
				SETFLOAT(&(toout[1]), i);
                SETFLOAT(&(toout[2]), x->mass[i].posX);
                outlet_anything(x->main_outlet, gensym("massPos"), 3, toout);
            }
        } 
    }
    else
    if (argc == 0)
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].posX);
            outlet_anything(x->main_outlet, gensym("massPos"), 3, toout);
        } 
    }
}
      
void pmpd_massSpeed(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_atom  toout[3];

    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].speedX);
            outlet_anything(x->main_outlet, gensym("massSpeed"), 3, toout);
        }  
    }
    else
    if ((argc>0)&&(argv[0].a_type == A_SYMBOL))
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), atom_getsymbolarg(0,argc,argv));
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
				SETFLOAT(&(toout[1]), i);
                SETFLOAT(&(toout[2]), x->mass[i].speedX);
                outlet_anything(x->main_outlet, gensym("massSpeed"), 3, toout);
            }
        } 
    }
    else
    if (argc == 0)
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].speedX);
            outlet_anything(x->main_outlet, gensym("massSpeed"), 3, toout);
        } 
    }
}

void pmpd_massForce(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{	
    int i;
    t_atom  toout[3];

    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].forceX);
            outlet_anything(x->main_outlet, gensym("massForce"), 3, toout);
        }  
    }
    else
    if ((argc>0)&&(argv[0].a_type == A_SYMBOL))
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), atom_getsymbolarg(0,argc,argv));
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
				SETFLOAT(&(toout[1]), i);
                SETFLOAT(&(toout[2]), x->mass[i].forceX);
                outlet_anything(x->main_outlet, gensym("massForce"), 3, toout);
            }
        } 
    }
    else
    if (argc == 0)
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].forceX);
            outlet_anything(x->main_outlet, gensym("massForce"), 3, toout);
        } 
    }
}

void pmpd_linkEnd(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
    t_atom  toout[4];
    
    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_link) )
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
            outlet_anything(x->main_outlet, gensym("linkEnd"), 4, toout);
        }
    }
    else
    if ((argc>0)&&(argv[0].a_type == A_SYMBOL))
    {
        SETSYMBOL(&(toout[0]), atom_getsymbolarg(0,argc,argv));
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(toout[1]), i);
                SETFLOAT(&(toout[2]), x->link[i].mass1->posX);
                SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
                outlet_anything(x->main_outlet, gensym("linkEnd"), 4, toout);
            }
        } 
    }
    else
    if (argc == 0) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
            outlet_anything(x->main_outlet, gensym("linkEnd"), 4, toout);
        } 
    }
}

void pmpd_linkPos(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
    t_atom  toout[3];
    
    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_link) )
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), (x->link[i].mass1->posX+x->link[i].mass2->posX)/2);
            outlet_anything(x->main_outlet, gensym("linkPos"), 3, toout);
        }
    }
    else
    if ((argc>0)&&(argv[0].a_type == A_SYMBOL))
    {
        SETSYMBOL(&(toout[0]), atom_getsymbolarg(0,argc,argv));
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(toout[1]), i);
				SETFLOAT(&(toout[2]), (x->link[i].mass1->posX+x->link[i].mass2->posX)/2);
                outlet_anything(x->main_outlet, gensym("linkPos"), 3, toout);
            }
        } 
    }
    else
    if (argc == 0) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
			SETFLOAT(&(toout[2]), (x->link[i].mass1->posX+x->link[i].mass2->posX)/2);
            outlet_anything(x->main_outlet, gensym("linkPos"), 3, toout);
        } 
    }
}

void pmpd_linkLength(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
    t_atom  toout[3];    

    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_link) )
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->link[i].mass1->posX-x->link[i].mass2->posX);
            outlet_anything(x->main_outlet, gensym("linkLength"), 3, toout);
        }
    }
    else
    if ((argc>0)&&(argv[0].a_type == A_SYMBOL))
    {
        SETSYMBOL(&(toout[0]), atom_getsymbolarg(0,argc,argv));
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
	            SETSYMBOL(&(toout[0]), x->link[i].Id);
	            SETFLOAT(&(toout[1]), i);
	            SETFLOAT(&(toout[2]), x->link[i].mass1->posX-x->link[i].mass2->posX);
	            outlet_anything(x->main_outlet, gensym("linkLength"), 3, toout);
            }
        } 
    }
    else
    if (argc == 0) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->link[i].mass1->posX-x->link[i].mass2->posX);
            outlet_anything(x->main_outlet, gensym("linkLength"), 3, toout);
        } 
    }
}

