int test_1d_mass(int i, t_pmpd *x, int argc, t_atom *argv)
{
	t_float tmp;
	t_int j = 1, k;
	while (j < argc) 
	{
		if (argv[j].a_type != A_SYMBOL) 
		{ 
			j++;
		}
		else
		{
			if (atom_getsymbolarg(j,argc,argv) == gensym("Id") )
			{
				if ( x->mass[i].Id != atom_getsymbolarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
            else if (atom_getsymbolarg(j,argc,argv) == gensym("mobile") )
			{
				if ( x->mass[i].mobile != atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("posXSup") )
			{
				if ( x->mass[i].posX < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("posXInf") )
			{
				if ( x->mass[i].posX >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("speedXSup") )
			{
				if ( x->mass[i].speedX < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("speedXInf") )
			{
				if ( x->mass[i].speedX >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("speedSup") )
			{
				if ( x->mass[i].speedX < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+= 2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("speedInf") )
			{
				if ( x->mass[i].speedX >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("distanceInf"))
			{ 
				tmp = sqr(x->mass[i].posX - atom_getfloatarg(j+1,argc,argv));
				if ( tmp >= sqr(atom_getfloatarg(j+2,argc,argv)) ) { return(0); }
				j += 3;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("distanceSup"))
			{ 
				tmp = sqr(x->mass[i].posX - atom_getfloatarg(j+1,argc,argv));
				if ( tmp < sqr(atom_getfloatarg(j+4,argc,argv)) ) { return(0); }
				j += 3;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("massSup"))
			{ 
				if ( 1/ x->mass[i].invM < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j += 2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("massInf"))
			{ 
				if ( 1/ x->mass[i].invM >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j += 2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("forceSup") )
			{
				if ( x->mass[i].forceX < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+= 2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("forceInf") )
			{
				if ( x->mass[i].forceX >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+= 2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("forceXSup") )
			{
				if ( x->mass[i].forceX < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("forceXInf") )
			{
				if ( x->mass[i].forceX >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("nbLinkSup") )
			{ // link number
				tmp=0;
				for (k=0; k < x->nb_link; k++)
				{
					if ( (x->link[k].mass1->num == x->mass[i].num) ||  (x->link[k].mass2->num == x->mass[i].num) ) tmp++;
				}
				if ( tmp <= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("nbLinkInf") )
			{ // link number
				tmp=0;
				for (k=0; k < x->nb_link; k++)
				{
					if ( (x->link[k].mass1->num == x->mass[i].num) ||  (x->link[k].mass2->num == x->mass[i].num) ) tmp++;
				}
				if ( tmp >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("nbLinkEqual") )
			{ // link number
				tmp=0;
				for (k=0; k < x->nb_link; k++)
				{
					if ( (x->link[k].mass1->num == x->mass[i].num) ||  (x->link[k].mass2->num == x->mass[i].num) ) tmp++;
				}
				if ( tmp != atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("nbLinkNameSup") )
			{ // link name, link number
				tmp=0;
				for (k=0; k < x->nb_link; k++)
				{
					if ( (x->link[k].Id ==  atom_getsymbolarg(j+1,argc,argv)) && ((x->link[k].mass2->num == x->mass[i].num) ||  (x->link[k].mass1->num == x->mass[i].num)) ) tmp++;
				}
				if ( tmp <= atom_getfloatarg(j+2,argc,argv) ) { return(0); }
				j+=3;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("nbLinkNameInf") )
			{ // link name, link number
				tmp=0;
				for (k=0; k < x->nb_link; k++)
				{
					if ( (x->link[k].Id ==  atom_getsymbolarg(j+1,argc,argv)) && ((x->link[k].mass2->num == x->mass[i].num) ||  (x->link[k].mass1->num == x->mass[i].num)) ) tmp++;
				}
				if ( tmp >= atom_getfloatarg(j+2,argc,argv) ) { return(0); }
				j+=3;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("nbLinkNameEqual") )
			{ // link name, link number
				tmp=0;
				for (k=0; k < x->nb_link; k++)
				{
					if ( (x->link[k].Id ==  atom_getsymbolarg(j+1,argc,argv)) && ((x->link[k].mass2->num == x->mass[i].num) ||  (x->link[k].mass1->num == x->mass[i].num)) ) tmp++;
				}
				if ( tmp != atom_getfloatarg(j+2,argc,argv) ) { return(0); }
				j+=3;
			}
			else
			{
				pd_error(x,"Option \"%s\" not reconized, stoping test",atom_getsymbolarg(j,argc,argv)->s_name);
				return(-1);
			}
		}
	}
	return(1);	
}

int test_1d_link(int i, t_pmpd *x, int argc, t_atom *argv)
{
	t_int j;
	t_float tmp;
	j = 1;
	
	while (j < argc) 
	{
		if (argv[j].a_type != A_SYMBOL) 
		{ 
			j++;
		}
		else
		{
			if (atom_getsymbolarg(j,argc,argv) == gensym("Id") )
			{
				if ( x->link[i].Id != atom_getsymbolarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}			
            else if (atom_getsymbolarg(j,argc,argv) == gensym("active") )
			{
				if ( x->link[i].active != atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("forceXSup") )
			{
				if ( x->link[i].forceX < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("forceXInf") )
			{
				if ( x->link[i].forceX >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("forceSup") )
			{	
				if ( x->link[i].forceX < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("forceInf") )
			{
				if ( x->link[i].forceX >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if (atom_getsymbolarg(j,argc,argv) == gensym("lengthSup") )
			{
				if ( x->link[i].distance < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("lengthInf") )
			{
				if ( x->link[i].distance >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("lengthXInf") )
			{
				tmp = fabs(x->link[i].mass1->posX - x->link[i].mass2->posX);
				if ( tmp >= atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("lengthXSup") )
			{
				tmp = fabs(x->link[i].mass1->posX - x->link[i].mass2->posX);
				if ( tmp < atom_getfloatarg(j+1,argc,argv) ) { return(0); }
				j+=2;
			}		
			else if ( atom_getsymbolarg(j,argc,argv) == gensym("connectedTo") )
			{
				if (argv[j+1].a_type == A_SYMBOL)
				{
					if (!( (x->link[i].mass1->Id == atom_getsymbolarg(j+1,argc,argv)) || (x->link[i].mass2->Id == atom_getsymbolarg(j+1,argc,argv)) )) 
					{ return(0); }
					j+=2;				
				}
				else if (argv[j+1].a_type == A_FLOAT)
				{
					if (!( (x->link[i].mass1->num == atom_getfloatarg(j+1,argc,argv)) || (x->link[i].mass2->num == atom_getfloatarg(j+1,argc,argv)) ))
					{ return(0); }
					j+=2;
				}
				else
				{
					pd_error(x,"bad argument for connectedTo");
					j+=1;
				}
			}
			else
			{
				pd_error(x,"Option \"%s\" not reconized, stoping test",atom_getsymbolarg(j,argc,argv)->s_name);
				return(-1);
			}
		}
	}
	return(1);
}

//----------------------------------------------------------------------

void pmpd_testLink(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	t_int i, tmp;
	t_atom std_out[2];

	SETSYMBOL(&(std_out[0]),atom_getsymbolarg(0,argc,argv));

	for (i=0; i < x->nb_link; i++)
	{
		tmp=test_1d_link(i,x,argc,argv);
		if (tmp == -1)
		{	
			break;
		}
		else if (tmp)
		{
			SETFLOAT(&(std_out[1]),i);
			outlet_anything(x->main_outlet, gensym("testLink"),2,std_out);
		}
	}
}

void pmpd_testMass(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	t_int i, tmp;
	t_atom std_out[2];

	SETSYMBOL(&(std_out[0]),atom_getsymbolarg(0,argc,argv));

	for (i=0; i < x->nb_mass; i++)
	{
		tmp=test_1d_mass(i,x,argc,argv);
		if (tmp == -1)
		{	
			break;
		}
		else if (tmp)
		{
			SETFLOAT(&(std_out[1]),i);
			outlet_anything(x->main_outlet, gensym("testMass"),2,std_out);
		}
	}
}

void pmpd_testLinkT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, vecsize, tmp;
    t_garray *a;
    t_word *vec;
    
    if (argv[0].a_type == A_SYMBOL)
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
            
            for (i=0; i < taille_max; i++)
            {
				tmp = test_1d_link(i,x,argc,argv);
				if (tmp == -1)
				{	
					break;
				}
				vec[i].w_float = (t_float)tmp;
            }
            garray_redraw(a);
        }
    }
}

void pmpd_testMassT(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, vecsize, tmp;
    t_garray *a;
    t_word *vec;
    
    if (argv[0].a_type == A_SYMBOL)
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
            
            for (i=0; i < taille_max; i++)
            {
				tmp = test_1d_mass(i,x,argc,argv);
				if (tmp == -1)
				{
					break;
				}
				vec[i].w_float = (t_float)tmp;
            }
            garray_redraw(a);
        }
    }
}

void pmpd_testLinkL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, tmp;
    t_atom list[x->nb_link+1];

    for (i=0; i < x->nb_link;)
	{
		tmp=test_1d_link(i,x,argc,argv);
		if (tmp == -1)
		{
			break;
		}
		SETFLOAT(&(list[i+1]), (t_float)tmp);
		i++;
	}
	SETSYMBOL(&(list[0]),atom_getsymbolarg(0,argc,argv));
	outlet_anything(x->main_outlet, gensym("testLinkL"),i+1 , list);
}

void pmpd_testMassL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, tmp;
    t_atom list[x->nb_mass+1];

    for (i=0; i < x->nb_mass;)
	{		
		tmp=test_1d_mass(i,x,argc,argv);
		if (tmp == -1)
		{
			break;
		}
		SETFLOAT(&(list[i+1]), (t_float)tmp);
		i++;
	}
	SETSYMBOL(&(list[0]),atom_getsymbolarg(0,argc,argv));
	outlet_anything(x->main_outlet, gensym("testMassL"),i+1 , list);
}

void pmpd_testLinkN(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	t_atom std_out[3];
	t_int i, tmp;

	SETSYMBOL(&(std_out[0]),atom_getsymbolarg(0,argc,argv));
    i = atom_getfloatarg(1,argc,argv);
    i = min(x->nb_link-1,i);
    i = max(0,i);
    SETFLOAT(&(std_out[1]),i);

    tmp=test_1d_link(i,x,argc,argv);
    if (tmp == -1)
    {	
        return;
    }
    if (tmp)
        SETFLOAT(&(std_out[2]),1);
    else
        SETFLOAT(&(std_out[2]),0);

    outlet_anything(x->main_outlet, gensym("testLinkN"),3,std_out);
}

void pmpd_testMassN(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	t_atom std_out[3];
	t_int i, tmp;

	SETSYMBOL(&(std_out[0]),atom_getsymbolarg(0,argc,argv));
    i = atom_getfloatarg(1,argc,argv);
    i = min(x->nb_mass-1,i);
    i = max(0,i);
    SETFLOAT(&(std_out[1]),i);

    tmp=test_1d_mass(i,x,argc,argv);
    if (tmp == -1)
    {	
        return;
    }
    if (tmp)
        SETFLOAT(&(std_out[2]),1);
    else
        SETFLOAT(&(std_out[2]),0);

    outlet_anything(x->main_outlet, gensym("testMassN"),3,std_out);
}

void pmpd_testLinkNumber(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	t_int i, tmp, nb;
	t_atom std_out[2];

	SETSYMBOL(&(std_out[0]),atom_getsymbolarg(0,argc,argv));

    nb=0;
	for (i=0; i < x->nb_link; i++)
	{
		tmp=test_1d_link(i,x,argc,argv);
		if (tmp == -1)
		{	
			break;
		}
		else if (tmp)
		{
            nb++;
		}
	}
    SETFLOAT(&(std_out[1]),nb);
    outlet_anything(x->main_outlet, gensym("testLinkNumber"),2,std_out);
}

void pmpd_testMassNumber(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
	t_int i, tmp, nb;
	t_atom std_out[2];

	SETSYMBOL(&(std_out[0]),atom_getsymbolarg(0,argc,argv));

    nb = 0;
	for (i=0; i < x->nb_mass; i++)
	{
		tmp=test_1d_mass(i,x,argc,argv);
		if (tmp == -1)
		{	
			break;
		}
		else if (tmp)
		{
            nb++;
		}
	}
    SETFLOAT(&(std_out[1]),nb);
	outlet_anything(x->main_outlet, gensym("testMassNumber"),2,std_out);
}
