void pmpd_massPosXL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
        for (i=0; i < x->nb_mass; i++)
        {
            SETFLOAT(&(pos_list[i]),x->mass[i].posX);
        }
        outlet_anything(x->main_outlet, gensym("massPosXL"),x->nb_mass , pos_list);        
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[i].posX);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massPosXL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].posX);
        outlet_anything(x->main_outlet, gensym("massPosXL"),1 , pos_list);        
    }
}

void pmpd_massForceXL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
 
    if (argc==0) 
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETFLOAT(&(pos_list[i]),x->mass[i].forceX);
        }
        outlet_anything(x->main_outlet, gensym("massForceXL"),x->nb_mass , pos_list);          
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[j].forceX);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massForceXL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].forceX);
        outlet_anything(x->main_outlet, gensym("massForceXL"),1 , pos_list);        
    }
}

void pmpd_massSpeedXL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
	
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++)
		{
			SETFLOAT(&(pos_list[i]),x->mass[i].speedX);
		}
		outlet_anything(x->main_outlet, gensym("massSpeedXL"),x->nb_mass , pos_list);         
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[j].speedX);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massSpeedXL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].speedX);
        outlet_anything(x->main_outlet, gensym("massSpeedXL"),1 , pos_list);        
    }
}

// ---------------------------------------------------------------------

void pmpd_linkPosXL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),(x->link[i].mass1->posX + x->link[i].mass2->posX)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosXL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),(x->link[i].mass1->posX + x->link[i].mass2->posX)/2);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosXL"),j , pos_list);
    }
}

void pmpd_linkLengthXL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),x->link[i].mass2->posX - x->link[i].mass1->posX);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthXL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),x->link[i].mass2->posX - x->link[i].mass1->posX);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkLengthXL"),j , pos_list);
    }
}

void pmpd_linkPosSpeedXL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),(x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosSpeedXL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),(x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosSpeedXL"),j , pos_list);
    }
}

void pmpd_linkLengthSpeedXL(t_pmpd *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedXL"), x->nb_link, pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedXL"), j, pos_list);
    }
}

