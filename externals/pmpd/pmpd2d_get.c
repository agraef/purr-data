void pmpd2d_get(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_symbol *toget; 
    t_atom  toout[5];
    toget = atom_getsymbolarg(0, argc, argv);

    if ( (toget == gensym("massesPos")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->mass[i].posX);
            SETFLOAT(&(toout[2]), x->mass[i].posY);
            outlet_anything(x->main_outlet, gensym("massesPosNo"), 3, toout);
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
                SETFLOAT(&(toout[2]), x->mass[i].posY);
                outlet_anything(x->main_outlet, gensym("massesPosId"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].posY);
            outlet_anything(x->main_outlet, gensym("massesPos"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].posY);
            outlet_anything(x->main_outlet, gensym("massesPosNameNo"), 3, toout);
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
                SETFLOAT(&(toout[2]), x->mass[i].posY);
                outlet_anything(x->main_outlet, gensym("massesPosNameId"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].posY);
            outlet_anything(x->main_outlet, gensym("massesPosName"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].speedY);
            outlet_anything(x->main_outlet, gensym("massesSpeedsNo"), 3, toout);
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
                SETFLOAT(&(toout[2]), x->mass[i].speedY);
                outlet_anything(x->main_outlet, gensym("massesSpeedsId"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].speedY);
            outlet_anything(x->main_outlet, gensym("massesSpeeds"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].speedY);
            outlet_anything(x->main_outlet, gensym("massesSpeedsNameNo"), 3, toout);
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
                SETFLOAT(&(toout[2]), x->mass[i].speedY);
                outlet_anything(x->main_outlet, gensym("massesSpeedsNameId"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].speedY);
            outlet_anything(x->main_outlet, gensym("massesSpeedsName"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].forceY);
            outlet_anything(x->main_outlet, gensym("massesForcesNo"), 3, toout);
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
                SETFLOAT(&(toout[2]), x->mass[i].forceY);
                outlet_anything(x->main_outlet, gensym("massesForcesId"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].forceY);
            outlet_anything(x->main_outlet, gensym("massesForces"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].forceY);
            outlet_anything(x->main_outlet, gensym("massesForcesNameNo"), 3, toout);
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
                SETFLOAT(&(toout[2]), x->mass[i].forceY);
                outlet_anything(x->main_outlet, gensym("massesForcesNameId"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->mass[i].forceY);
            outlet_anything(x->main_outlet, gensym("massesForcesName"), 3, toout);
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
            SETFLOAT(&(toout[2]), x->link[i].mass1->posY);
            SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
            SETFLOAT(&(toout[4]), x->link[i].mass2->posY);
            outlet_anything(x->main_outlet, gensym("linksPosNo"), 5, toout);
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
                SETFLOAT(&(toout[2]), x->link[i].mass1->posY);
                SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
                SETFLOAT(&(toout[4]), x->link[i].mass2->posY);
                outlet_anything(x->main_outlet, gensym("linksPosNo"), 5, toout);
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
            SETFLOAT(&(toout[2]), x->link[i].mass1->posY);
            SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
            SETFLOAT(&(toout[4]), x->link[i].mass2->posY);
            outlet_anything(x->main_outlet, gensym("linksPosNo"), 5, toout);
        } 
    }
    else
    if ( (toget == gensym("linksPosName")) && (argv[1].a_type == A_FLOAT) )
    {
        i = atom_getfloatarg(1, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[2]), x->link[i].mass1->posY);
            SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
            SETFLOAT(&(toout[4]), x->link[i].mass2->posY);
            outlet_anything(x->main_outlet, gensym("linksPosNo"), 5, toout);
        }
    }
    else
    if ( (toget == gensym("linksPosName")) && (argv[1].a_type == A_SYMBOL) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->link[i].Id)
            {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[2]), x->link[i].mass1->posY);
            SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
            SETFLOAT(&(toout[4]), x->link[i].mass2->posY);
            outlet_anything(x->main_outlet, gensym("linksPosNo"), 5, toout);
            }
        } 
    }
    else
    if ( (toget == gensym("linksPosName")) && (argc == 1) )
    {
        for (i=0; i< x->nb_link; i++)
        {
            SETFLOAT(&(toout[0]), i);
            SETFLOAT(&(toout[1]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[2]), x->link[i].mass1->posY);
            SETFLOAT(&(toout[3]), x->link[i].mass2->posX);
            SETFLOAT(&(toout[4]), x->link[i].mass2->posY);
            outlet_anything(x->main_outlet, gensym("linksPosNo"), 5, toout);
        } 
    }
    else
        error("not get attribute");
}

void pmpd2d_massPos(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_atom  toout[4];

    if ((argc>0)&&(argv[0].a_type == A_FLOAT))
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].posX);
            SETFLOAT(&(toout[3]), x->mass[i].posY);
            outlet_anything(x->main_outlet, gensym("massPos"), 4, toout);
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
                SETFLOAT(&(toout[3]), x->mass[i].posY);
                outlet_anything(x->main_outlet, gensym("massPos"), 4, toout);
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
            SETFLOAT(&(toout[3]), x->mass[i].posY);
            outlet_anything(x->main_outlet, gensym("massPos"), 4, toout);
        } 
    }
}
      
void pmpd2d_massSpeed(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_atom  toout[4];

    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].speedX);
            SETFLOAT(&(toout[3]), x->mass[i].speedY);
            outlet_anything(x->main_outlet, gensym("massSpeed"), 4, toout);
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
                SETFLOAT(&(toout[3]), x->mass[i].speedY);
                outlet_anything(x->main_outlet, gensym("massSpeed"), 4, toout);
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
            SETFLOAT(&(toout[3]), x->mass[i].speedY);
            outlet_anything(x->main_outlet, gensym("massSpeed"), 4, toout);
        } 
    }
}

void pmpd2d_massForce(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{	
    int i;
    t_atom  toout[4];

    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_mass) )
        {
            SETSYMBOL(&(toout[0]), x->mass[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->mass[i].forceX);
            SETFLOAT(&(toout[3]), x->mass[i].forceY);
            outlet_anything(x->main_outlet, gensym("massForce"), 4, toout);
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
                SETFLOAT(&(toout[3]), x->mass[i].forceY);
                outlet_anything(x->main_outlet, gensym("massForce"), 4, toout);
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
            SETFLOAT(&(toout[3]), x->mass[i].forceY);
            outlet_anything(x->main_outlet, gensym("massForce"), 4, toout);
        } 
    }
}

void pmpd2d_linkEnd(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
    t_atom  toout[6];
    
    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_link) )
        {
            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), x->link[i].mass1->posX);
            SETFLOAT(&(toout[3]), x->link[i].mass1->posY);
            SETFLOAT(&(toout[4]), x->link[i].mass2->posX);
            SETFLOAT(&(toout[5]), x->link[i].mass2->posY);
            outlet_anything(x->main_outlet, gensym("linkEnd"), 6, toout);
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
                SETFLOAT(&(toout[3]), x->link[i].mass1->posY);
                SETFLOAT(&(toout[4]), x->link[i].mass2->posX);
                SETFLOAT(&(toout[5]), x->link[i].mass2->posY);
                outlet_anything(x->main_outlet, gensym("linkEnd"), 6, toout);
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
            SETFLOAT(&(toout[3]), x->link[i].mass1->posY);
            SETFLOAT(&(toout[4]), x->link[i].mass2->posX);
            SETFLOAT(&(toout[5]), x->link[i].mass2->posY);
            outlet_anything(x->main_outlet, gensym("linkEnd"), 6, toout);
        } 
    }
}

void pmpd2d_linkPos(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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
            SETFLOAT(&(toout[2]), (x->link[i].mass1->posX+x->link[i].mass2->posX)/2);
            SETFLOAT(&(toout[3]), (x->link[i].mass1->posY+x->link[i].mass2->posY)/2);
            outlet_anything(x->main_outlet, gensym("linkPos"), 4, toout);
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
				SETFLOAT(&(toout[3]), (x->link[i].mass1->posY+x->link[i].mass2->posY)/2);
                outlet_anything(x->main_outlet, gensym("linkPos"), 4, toout);
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
			SETFLOAT(&(toout[2]), (x->link[i].mass1->posX-x->link[i].mass2->posX));
			SETFLOAT(&(toout[3]), (x->link[i].mass1->posY+x->link[i].mass2->posY));
            outlet_anything(x->main_outlet, gensym("linkPos"), 4, toout);
        } 
    }
}

void pmpd2d_linkLength(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
	int i;
    t_atom  toout[5];    
    t_float tmp1, tmp2;

    if ((argc>0)&&(argv[0].a_type == A_FLOAT)) 
    {
        i = atom_getfloatarg(0, argc, argv);
        if ( (i>=0) && (i<x->nb_link) )
        {
            tmp1=x->link[i].mass1->posX-x->link[i].mass2->posX;
            tmp2=x->link[i].mass1->posY-x->link[i].mass2->posY;

            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), tmp1);
            SETFLOAT(&(toout[3]), tmp2);
            SETFLOAT(&(toout[4]), sqrt(tmp1*tmp1 + tmp2*tmp2));
            outlet_anything(x->main_outlet, gensym("linkLength"), 5, toout);
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
                tmp1=x->link[i].mass1->posX-x->link[i].mass2->posX;
                tmp2=x->link[i].mass1->posY-x->link[i].mass2->posY;

				SETFLOAT(&(toout[1]), i);
                SETFLOAT(&(toout[2]), tmp1);
                SETFLOAT(&(toout[3]), tmp2);
                SETFLOAT(&(toout[4]), sqrt(tmp1*tmp1 + tmp2*tmp2));
                outlet_anything(x->main_outlet, gensym("linkLength"), 5, toout);
            }
        } 
    }
    else
    if (argc == 0) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            tmp1=x->link[i].mass1->posX-x->link[i].mass2->posX;
            tmp2=x->link[i].mass1->posY-x->link[i].mass2->posY;

            SETSYMBOL(&(toout[0]), x->link[i].Id);
            SETFLOAT(&(toout[1]), i);
            SETFLOAT(&(toout[2]), tmp1);
            SETFLOAT(&(toout[3]), tmp2);
            SETFLOAT(&(toout[4]), sqrt(tmp1*tmp1 + tmp2*tmp2));
            outlet_anything(x->main_outlet, gensym("linkLength"), 5, toout);
        } 
    }
}

