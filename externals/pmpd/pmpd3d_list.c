void pmpd3d_massPosL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[3*x->nb_mass];
    
    if (argc==0) 
    {
		for (i=0; i < x->nb_mass; i++)
		{
			SETFLOAT(&(pos_list[3*i]  ),x->mass[i].posX);
			SETFLOAT(&(pos_list[3*i+1]),x->mass[i].posY);
			SETFLOAT(&(pos_list[3*i+2]),x->mass[i].posZ);
		}
		outlet_anything(x->main_outlet, gensym("massPosL"),3*x->nb_mass , pos_list);       
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
            {
				SETFLOAT(&(pos_list[3*i]  ),x->mass[j].posX);
				SETFLOAT(&(pos_list[3*i+1]),x->mass[j].posY);
				SETFLOAT(&(pos_list[3*i+2]),x->mass[j].posZ);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massPosL"),3*i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].posX);
		SETFLOAT(&(pos_list[1]),x->mass[(int)atom_getfloatarg(0, argc, argv)].posY);
		SETFLOAT(&(pos_list[2]),x->mass[(int)atom_getfloatarg(0, argc, argv)].posZ);
			
        outlet_anything(x->main_outlet, gensym("massPosL"),3 , pos_list);        
    }
}

void pmpd3d_massForceL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[3*x->nb_mass];
 
    if (argc==0) 
    {
        for (i=0; i< x->nb_mass; i++) {
        SETFLOAT(&(pos_list[3*i]  ),x->mass[i].forceX);
        SETFLOAT(&(pos_list[3*i+1]),x->mass[i].forceY);
        SETFLOAT(&(pos_list[3*i+2]),x->mass[i].forceZ);
        }
        outlet_anything(x->main_outlet, gensym("massForceL"),3*x->nb_mass , pos_list);          
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
            {
				SETFLOAT(&(pos_list[3*i]  ),x->mass[j].forceX);
				SETFLOAT(&(pos_list[3*i+1]),x->mass[j].forceY);
				SETFLOAT(&(pos_list[3*i+2]),x->mass[j].forceZ);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massForceL"),3*i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].forceX);
        SETFLOAT(&(pos_list[1]),x->mass[(int)atom_getfloatarg(0, argc, argv)].forceY);
        SETFLOAT(&(pos_list[2]),x->mass[(int)atom_getfloatarg(0, argc, argv)].forceZ);
        
        outlet_anything(x->main_outlet, gensym("massForceL"),3 , pos_list);        
    }
}

void pmpd3d_massSpeedL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[3*x->nb_mass];
	
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++) {
			SETFLOAT(&(pos_list[3*i]  ),x->mass[i].speedX);
			SETFLOAT(&(pos_list[3*i+1]),x->mass[i].speedY);
			SETFLOAT(&(pos_list[3*i+2]),x->mass[i].speedZ);
		}
		outlet_anything(x->main_outlet, gensym("massSpeedL"),3*x->nb_mass , pos_list);         
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			    {
				SETFLOAT(&(pos_list[3*i]  ),x->mass[j].speedX);
				SETFLOAT(&(pos_list[3*i+1]),x->mass[j].speedY);
				SETFLOAT(&(pos_list[3*i+2]),x->mass[j].speedZ);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massSpeedL"),3*i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].speedX);
        SETFLOAT(&(pos_list[1]),x->mass[(int)atom_getfloatarg(0, argc, argv)].speedY);
        SETFLOAT(&(pos_list[2]),x->mass[(int)atom_getfloatarg(0, argc, argv)].speedZ);
        
        outlet_anything(x->main_outlet, gensym("massSpeedL"),3 , pos_list);        
    }
}

void pmpd3d_massPosXL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd3d_massForceXL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd3d_massSpeedXL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd3d_massPosYL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
        for (i=0; i < x->nb_mass; i++)
        {
            SETFLOAT(&(pos_list[i]),x->mass[i].posY);
        }
        outlet_anything(x->main_outlet, gensym("massPosYL"),x->nb_mass , pos_list);         
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[j].posY);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massPosYL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].posY);
        outlet_anything(x->main_outlet, gensym("massPosYL"),1 , pos_list);        
    }
}

void pmpd3d_massForceYL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
 
    if (argc==0) 
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETFLOAT(&(pos_list[i]),x->mass[i].forceY);
        }
        outlet_anything(x->main_outlet, gensym("massForceYL"),x->nb_mass , pos_list);          
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[j].forceY);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massForceYL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].forceY);
        outlet_anything(x->main_outlet, gensym("massForceYL"),1 , pos_list);        
    }
}

void pmpd3d_massSpeedYL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
	
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++)
		{
			SETFLOAT(&(pos_list[i]),x->mass[i].speedY);
		}
		outlet_anything(x->main_outlet, gensym("massSpeedYL"),x->nb_mass , pos_list);         
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[j].speedY);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massSpeedYL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].speedY);
        outlet_anything(x->main_outlet, gensym("massSpeedYL"),1 , pos_list);        
    }
}

void pmpd3d_massPosZL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
        for (i=0; i < x->nb_mass; i++)
        {
            SETFLOAT(&(pos_list[i]),x->mass[i].posZ);
        }
        outlet_anything(x->main_outlet, gensym("massPosZL"),x->nb_mass , pos_list);         
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[j].posZ);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massPosZL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].posZ);
        outlet_anything(x->main_outlet, gensym("massPosZL"),1 , pos_list);        
    }
}

void pmpd3d_massForceZL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
 
    if (argc==0) 
    {
        for (i=0; i< x->nb_mass; i++)
        {
            SETFLOAT(&(pos_list[i]),x->mass[i].forceZ);
        }
        outlet_anything(x->main_outlet, gensym("massForceZL"),x->nb_mass , pos_list);          
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[j].forceZ);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massForceZL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].forceZ);
        outlet_anything(x->main_outlet, gensym("massForceZL"),1 , pos_list);        
    }
}

void pmpd3d_massSpeedZL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
	
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++)
		{
			SETFLOAT(&(pos_list[i]),x->mass[i].speedZ);
		}
		outlet_anything(x->main_outlet, gensym("massSpeedZL"),x->nb_mass , pos_list);         
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
                SETFLOAT(&(pos_list[i]),x->mass[i].speedZ);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massSpeedZL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].speedZ);
        outlet_anything(x->main_outlet, gensym("massSpeedZL"),1 , pos_list);        
    }
}

// ---------------------------------------------------------------------

void pmpd3d_massPosNormL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++) {
			SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].posX)+sqr(x->mass[i].posY)+sqr(x->mass[i].posZ)));
		}
    outlet_anything(x->main_outlet, gensym("massPosNormL"),x->nb_mass , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
				SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[j].posX)+sqr(x->mass[j].posY)+sqr(x->mass[j].posZ)));
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massPosNormL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
		i=(int)atom_getfloatarg(0, argc, argv);
		SETFLOAT(&(pos_list[0]),sqrt(sqr(x->mass[i].posX)+sqr(x->mass[i].posY)+sqr(x->mass[i].posZ)));
        outlet_anything(x->main_outlet, gensym("massPosNormL"),1 , pos_list);        
    }
}

void pmpd3d_massForceNormL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{   
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++) 
		{
			SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].forceX)+sqr(x->mass[i].forceY)+sqr(x->mass[i].forceZ)));
		}
    outlet_anything(x->main_outlet, gensym("massForceNormL"),x->nb_mass , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
				SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[j].forceX)+sqr(x->mass[j].forceY)+sqr(x->mass[j].forceZ)));
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massForceNormL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
		i=(int)atom_getfloatarg(0, argc, argv);
        SETFLOAT(&(pos_list[0]),sqrt(sqr(x->mass[i].forceX)+sqr(x->mass[i].forceY)+sqr(x->mass[i].forceZ)));
        outlet_anything(x->main_outlet, gensym("massForceNormL"),1 , pos_list);        
    }
}

void pmpd3d_massSpeedNormL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++) 
		{
			SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].speedX)+sqr(x->mass[i].speedY)+sqr(x->mass[i].speedZ)));
		}
    outlet_anything(x->main_outlet, gensym("massSpeedNormL"),x->nb_mass , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {            
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			{
				SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[j].speedX)+sqr(x->mass[j].speedY)+sqr(x->mass[j].speedZ)));
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massSpeedNormL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
		i=(int)atom_getfloatarg(0, argc, argv);
        SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].speedX)+sqr(x->mass[i].speedY)+sqr(x->mass[i].speedZ)));
        outlet_anything(x->main_outlet, gensym("massSpeedNormL"),1 , pos_list);        
    }
}

// ---------------------------------------------------------------------

void pmpd3d_linkPosL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[3*x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[3*i]  ),(x->link[i].mass2->posX + x->link[i].mass1->posX)/2);
			SETFLOAT(&(pos_list[3*i+1]),(x->link[i].mass2->posY + x->link[i].mass1->posY)/2);
			SETFLOAT(&(pos_list[3*i+2]),(x->link[i].mass2->posZ + x->link[i].mass1->posZ)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosL"),3*x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[3*j]  ),(x->link[i].mass2->posX + x->link[i].mass1->posX)/2);
				SETFLOAT(&(pos_list[3*j+1]),(x->link[i].mass2->posY + x->link[i].mass1->posY)/2);
				SETFLOAT(&(pos_list[3*j+2]),(x->link[i].mass2->posZ + x->link[i].mass1->posZ)/2);
				j++;
            }
        }
  		outlet_anything(x->main_outlet, gensym("linkPosL"), 3*j, pos_list);   
    }
}

void pmpd3d_linkLengthL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[3*x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[3*i]  ),x->link[i].mass2->posX - x->link[i].mass1->posX);
			SETFLOAT(&(pos_list[3*i+1]),x->link[i].mass2->posY - x->link[i].mass1->posY);
			SETFLOAT(&(pos_list[3*i+2]),x->link[i].mass2->posZ - x->link[i].mass1->posZ);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthL"),3*x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[3*j]  ),x->link[i].mass2->posX - x->link[i].mass1->posX);
				SETFLOAT(&(pos_list[3*j+1]),x->link[i].mass2->posY - x->link[i].mass1->posY);
				SETFLOAT(&(pos_list[3*j+2]),x->link[i].mass2->posZ - x->link[i].mass1->posZ);
				j++;
            }
        }
  		outlet_anything(x->main_outlet, gensym("linkLengthL"), 3*j, pos_list);   
    }
}

void pmpd3d_linkPosSpeedL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[3*x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[3*i]  ),(x->link[i].mass2->speedX + x->link[i].mass1->speedX)/2);
			SETFLOAT(&(pos_list[3*i+1]),(x->link[i].mass2->speedY + x->link[i].mass1->speedY)/2);
			SETFLOAT(&(pos_list[3*i+2]),(x->link[i].mass2->speedZ + x->link[i].mass1->speedZ)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosSpeedL"),3*x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[3*j]  ),(x->link[i].mass2->speedX + x->link[i].mass1->speedX)/2);
				SETFLOAT(&(pos_list[3*j+1]),(x->link[i].mass2->speedY + x->link[i].mass1->speedY)/2);
				SETFLOAT(&(pos_list[3*j+2]),(x->link[i].mass2->speedZ + x->link[i].mass1->speedZ)/2);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosSpeedL"),3*j , pos_list);
    }
}

void pmpd3d_linkLengthSpeedL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[3*x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[3*i]  ),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
			SETFLOAT(&(pos_list[3*i+1]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
			SETFLOAT(&(pos_list[3*i+2]),x->link[i].mass2->speedZ - x->link[i].mass1->speedZ);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedL"),3*x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[3*j]  ),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
				SETFLOAT(&(pos_list[3*j+1]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
				SETFLOAT(&(pos_list[3*j+2]),x->link[i].mass2->speedZ - x->link[i].mass1->speedZ);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedL"),3*j , pos_list);
    }
}

void pmpd3d_linkPosXL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd3d_linkLengthXL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd3d_linkPosSpeedXL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd3d_linkLengthSpeedXL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd3d_linkPosYL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),(x->link[i].mass1->posY + x->link[i].mass2->posY)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosYL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),(x->link[i].mass1->posY + x->link[i].mass2->posY)/2);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosYL"), j, pos_list);
    }
}

void pmpd3d_linkLengthYL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),x->link[i].mass2->posY - x->link[i].mass1->posY);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthYL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),x->link[i].mass2->posY - x->link[i].mass1->posY);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkLengthYL"), j, pos_list);
    }
}

void pmpd3d_linkPosSpeedYL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),(x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosSpeedYL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),(x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosSpeedYL"), j, pos_list);
    }
}

void pmpd3d_linkLengthSpeedYL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedYL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedYL"), j, pos_list);
    }
}

void pmpd3d_linkPosZL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),(x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosZL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),(x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosZL"), j, pos_list);
    }
}

void pmpd3d_linkLengthZL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),x->link[i].mass2->posZ - x->link[i].mass1->posZ);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthZL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),x->link[i].mass2->posZ - x->link[i].mass1->posZ);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkLengthZL"), j, pos_list);
    }
}

void pmpd3d_linkPosSpeedZL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),(x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosSpeedZL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),(x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosSpeedZL"), j, pos_list);
    }
}

void pmpd3d_linkLengthSpeedZL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),x->link[i].mass2->speedZ - x->link[i].mass1->speedZ);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedZL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),x->link[i].mass2->speedZ - x->link[i].mass1->speedZ);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedZL"), j, pos_list);
    }
}

// ---------------------------------------------------------------------

void pmpd3d_linkPosNormL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + \
                            sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + \
                            sqr((x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2) ));
		}
		outlet_anything(x->main_outlet, gensym("linkPosNormL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),sqrt( \
                            sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + \
                            sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + \
                            sqr((x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2) ));
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosNormL"), j, pos_list);
    }		
}

void pmpd3d_linkLengthNormL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr(x->link[i].mass2->posX - x->link[i].mass1->posX) + \
                            sqr(x->link[i].mass2->posY - x->link[i].mass1->posY) + \
                            sqr(x->link[i].mass2->posZ - x->link[i].mass1->posZ) ));
		}
		outlet_anything(x->main_outlet, gensym("linkLengthNormL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),sqrt( \
                            sqr(x->link[i].mass2->posX - x->link[i].mass1->posX) + \
                            sqr(x->link[i].mass2->posY - x->link[i].mass1->posY) + \
                            sqr(x->link[i].mass2->posZ - x->link[i].mass1->posZ) ));

				j++;
			}
        }
		outlet_anything(x->main_outlet, gensym("linkLengthNormL"), j, pos_list);
    }
}

void pmpd3d_linkPosSpeedNormL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2) + \
                            sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + \
                            sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2) ));
		}
		outlet_anything(x->main_outlet, gensym("linkPosSpeedNormL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),sqrt( \
                            sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2) + \
                            sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + \
                            sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2) ));
                j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosSpeedNormL"), j, pos_list);
    }
}

void pmpd3d_linkLengthSpeedNormL(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr(x->link[i].mass2->speedX - x->link[i].mass1->speedX) + \
                            sqr(x->link[i].mass2->speedY - x->link[i].mass1->speedY) + \
                            sqr(x->link[i].mass2->speedZ - x->link[i].mass1->speedZ) ));
		}
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedNormL"),x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[j]),sqrt( \
                            sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2) + \
                            sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + \
                            sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2) ));
				j++;
			}
        }
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedNormL"), j, pos_list);
    }
}
