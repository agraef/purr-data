void pmpd_infosL(t_pmpd *x)
{
    int i;
    post("list of %d mass", x->nb_mass);
    post("number, Id, mobile, mass, Damping, position, speed, forces");
    for(i=0; i < x->nb_mass; i++)
    {
        post("masse %i: %s, %d, %f, %f, %f, %f, %f",i, x->mass[i].Id->s_name, x->mass[i].mobile, 1/x->mass[i].invM, x->mass[i].D2, x->mass[i].posX, x->mass[i].speedX, x->mass[i].forceX );
    }

    post("list of %d link", x->nb_link);
    post("number, Id, mass1, mass2, K, D, Pow, L, Lmin, Lmax");
    for(i=0; i < x->nb_link; i++)
    {
        switch(x->link[i].lType)
        {
        case 0 :
            post("link %i: %s, %i, %i, %i, %f, %f, %f, %f, %f, %f", i, x->link[i].Id->s_name, x->link[i].active, x->link[i].mass1->num, x->link[i].mass2->num, x->link[i].K, x->link[i].D, x->link[i].Pow, x->link[i].L, x->link[i].Lmin, x->link[i].Lmax);
            break;
        case 2 :
            post("tabLink %i: %s, %i, %i, %i, %f, %f, %s, %f, %s, %f", i, x->link[i].Id->s_name, x->link[i].active, x->link[i].mass1->num, x->link[i].mass2->num, x->link[i].K, x->link[i].D, x->link[i].arrayK->s_name, x->link[i].K_L, x->link[i].arrayD->s_name, x->link[i].D_L);
            break;
        }
    }
}

void pmpd_forceX(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
// add a force to a specific mass
    int tmp, i;
    t_garray *a;
    int npoints, n;
    t_word *vec;
    t_float K;
    
    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].forceX += atom_getfloatarg(1, argc, argv);
    }
    else if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].forceX += atom_getfloatarg(1, argc, argv);
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
					x->mass[i].forceX += K*vec[n].w_float;
					n++;
					if (n >= npoints) break;
				}
			}
		}
	}
}

void pmpd_minX(t_pmpd *x, t_float min)
{
    x->minX = min;
}

void pmpd_maxX(t_pmpd *x, t_float max)
{
    x->maxX = max;
}

void pmpd_addPosX(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int tmp, i;

    if ( (argc == 2) && ( argv[0].a_type == A_FLOAT ) && ( argv[1].a_type == A_FLOAT ) )
    {
        tmp = atom_getfloatarg(0, argc, argv);
        tmp = max(0, min( x->nb_mass-1, tmp));
        x->mass[tmp].posX += atom_getfloatarg(1, argc, argv);
    }
    if ( (argc == 2) && ( argv[0].a_type == A_SYMBOL ) && ( argv[1].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            {
                x->mass[i].posX += atom_getfloatarg(1, argc, argv);
            }
        }
    }
}

//----------------------------------------------

void pmpd_grabMass(t_pmpd *x, t_float posX, t_float grab)
{
    t_float dist, tmp;
    t_int i;
    
    if (grab == 0)
        x->grab=0;
    if ((x->grab == 0)&(grab == 1)&(x->nb_mass > 0))
    {
        x->grab=1;
        x->grab_nb= 0;
        dist = x->mass[0].posX - posX;
        for (i=1; i<x->nb_mass; i++)
        {
            tmp = x->mass[i].posX - posX;
            if (tmp < dist)
            {
                dist = tmp;
                x->grab_nb= i;
            }
        }
    }
    if (x->grab == 1)
    {
        x->mass[x->grab_nb].posX = posX;
    }
}

void pmpd_closestMass(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float dist, tmp;
    t_int i;
    t_atom std_out[2];
    t_float posX;
    posX = atom_getfloatarg(0, argc, argv);
    
    if ( (argc == 2)  && (argv[1].a_type == A_SYMBOL) )
    {
        //t_symbol *mass_name = atom_getsymbolarg(2, argc, argv);
        if ((x->nb_mass > 0))
        {
            dist = 1000000000;//sqr(x->mass[0].posX - posX) + sqr(x->mass[0].posY - posY);
            for (i=0; i<x->nb_mass; i++)
            {
                if (atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
                {
                    tmp = x->mass[i].posX - posX;
                    if (tmp < dist)
                    {
                        dist = tmp;
                        x->grab_nb= i;
                    }
                }
            }
        }
    }
    else {
        if ((x->nb_mass > 0))
        {
            x->grab_nb= 0;
            dist = x->mass[0].posX - posX;
            for (i=1; i<x->nb_mass; i++)
            {
                tmp = x->mass[i].posX - posX;
                if (tmp < dist)
                {
                    dist = tmp;
                    x->grab_nb= i;
                }
            }
        }
    }

    SETFLOAT(&(std_out[0]),x->grab_nb);
    SETFLOAT(&(std_out[1]), x->mass[x->grab_nb].posX);
    outlet_anything(x->main_outlet, gensym("closestMass"),2,std_out);
}

void pmpd_massDistances_f_f(t_pmpd *x, t_int i, t_int j)
{
	t_float dist;
	t_atom to_out[3];

	dist = x->mass[i].posX - x->mass[j].posX;

	SETFLOAT(&(to_out[0]), i);
	SETFLOAT(&(to_out[1]), j);
	SETFLOAT(&(to_out[2]), dist);
	outlet_anything(x->main_outlet, gensym("distance"), 3, to_out);
}

void pmpd_massDistances(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	t_int i,j;

	if ( (argc == 2) && (argv[0].a_type == A_FLOAT) && (argv[1].a_type == A_FLOAT) )
	{
		pmpd_massDistances_f_f(x, atom_getfloatarg(0, argc, argv), atom_getfloatarg(1, argc, argv));
	}
	else if ( (argc == 2) && (argv[0].a_type == A_FLOAT) && (argv[1].a_type == A_SYMBOL) )
	{
		for (i=0; i < x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
			{
				pmpd_massDistances_f_f(x, atom_getfloatarg(0, argc, argv), i);
			}
		}
	}
	else if ( (argc == 2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_FLOAT) )
	{
		for (i=0; i < x->nb_mass; i++)
		{
			if ( (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id) )
			{
				pmpd_massDistances_f_f(x, atom_getfloatarg(1, argc, argv), i);
			}
		}
	}
	else if ( (argc == 2) && (argv[0].a_type == A_SYMBOL) && (argv[1].a_type == A_SYMBOL) )
	{
		for (i=0; i < x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{
				for (j=i+1; j < x->nb_mass; j++)
				{
					if ( atom_getsymbolarg(1,argc,argv) == x->mass[j].Id)
					{
						pmpd_massDistances_f_f(x,i, j);
					}
				}
			}
		}
	}
	else if (argc == 0)
	{
		for (i=0; i < x->nb_mass; i++)
		{
			for (j=i+1; j < x->nb_mass; j++)
			{
				pmpd_massDistances_f_f(x,i, j);
			}
		}
	}
	else if ((argc == 1) && (argv[0].a_type == A_SYMBOL) )
	{
		for (i=0; i < x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{
				for (j=i+1; j < x->nb_mass; j++)
				{
					pmpd_massDistances_f_f(x,i, j);
				}
			}
		}
	}
	else if ( (argc == 1) && (argv[0].a_type == A_FLOAT) )
	{
		for (i=0; i < x->nb_mass; i++)
		{
			pmpd_massDistances_f_f(x, atom_getfloatarg(0, argc, argv), i);
		}
	}
}

int bulle_order(t_int *listIndex, t_float *listDistance, t_int index)
{
    t_int tmpI;
    t_float tmpD;
    
    if ( listDistance[index] < listDistance[index+1] )
    {
        tmpD =  listDistance[index];
        tmpI =  listIndex[index];
        listDistance[index] = listDistance[index+1];
        listIndex[index] = listIndex[index+1];
        listDistance[index+1] = tmpD;
        listIndex[index+1]= tmpI;
        return(1);
    }
    return(0);
}

void pmpd_closestMassN(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	t_float dist;
	t_int i, j, nbout, list_index[x->nb_mass];
    t_float list_distance[x->nb_mass];
	t_atom std_out[x->nb_mass * 2];
	t_float posX;

	if ( (argc >= 1)  && (argv[0].a_type == A_FLOAT) )
        nbout = atom_getfloatarg(0, argc, argv);
    else
        nbout = 1;
    nbout = max(nbout,0);
    nbout = min(nbout,x->nb_mass);

	if ( (argc >= 2)  && (argv[1].a_type == A_FLOAT) )
        posX = atom_getfloatarg(1, argc, argv);
    else
        posX = 0;

    for (i=0; i < nbout; i++) // on remplie avec les premiere données disponible
    {
        list_index[i] = i;
        list_distance[i] = x->mass[i].posX - posX ;
    }

    for (i=1; i < nbout; i++) //trie a bulle pour ordoner cela
    {
        for (j=0; j < nbout-i; j++)
        {
            bulle_order(list_index, list_distance, j);
        } 
    }
  
    for (i = nbout; i< x->nb_mass; i++) // on test le reste des masses
    {
        dist = x->mass[i].posX - posX;
        if (dist < list_distance[0]) // cette mass doit rentrer dans la liste
        {
            list_index[0] = i;
            list_distance[0] = dist;
            j = 0;
            while ( (j<nbout-1) && bulle_order(list_index, list_distance, j) ) // on reordone la liste
            {
                j++;
            }
        }
    }
  
    for (i=0; i<nbout; i++)
    {
        SETFLOAT(&(std_out[2*i]), list_index[nbout-1-i]);
        SETFLOAT(&(std_out[2*i+1]), list_distance[nbout-1-i]);        
    }

	outlet_anything(x->main_outlet, gensym("closestMassN"),2*nbout,std_out);
}


/*
void pmpd_forcesXT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
// add forces to masses. forces comes from a table, masse can be filter on ther Id or not

	t_int i, j;
	t_garray *a;
	int npoints;
	t_word *vec;
	t_symbol *array;

	if ( ( argc > 0 ) && (argv[0].a_type == A_SYMBOL ))
	{
		array = atom_getsymbolarg(0,argc,argv);
		if (!(a = (t_garray *)pd_findbyclass(array, garray_class)))
			pd_error(x, "%s: no such array", array->s_name);
		else if (!garray_getfloatwords(a, &npoints, &vec))
			pd_error(x, "%s: bad template for tabLink", array->s_name);
		else
		{
			if ( argc == 1 )  // apply forces on all masses
			{
				j = min(x->nb_mass, npoints);
				for (i=0; i< j; i++)
				{
					x->mass[i].forceX += vec[i].w_float;
				}
			}
			else if (( argc > 1 ) && ( argv[1].a_type == A_SYMBOL ))
			{
				i=0;
				j=0;

				while ( (i < x->nb_mass) && (j < npoints) )
				{
					if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
					{
						x->mass[i].forceX += vec[j].w_float;
						j++;
					}
					i++;
				}
			}
		}
	}
}
*/
