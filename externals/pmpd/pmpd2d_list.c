void pmpd2d_massPosL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[2*x->nb_mass];
    
    if (argc==0) 
    {
		for (i=0; i < x->nb_mass; i++)
		{
			SETFLOAT(&(pos_list[2*i]  ),x->mass[i].posX);
			SETFLOAT(&(pos_list[2*i+1]),x->mass[i].posY);
		}
		outlet_anything(x->main_outlet, gensym("massPosL"),2*x->nb_mass , pos_list);       
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
            {
				SETFLOAT(&(pos_list[2*i]  ),x->mass[j].posX);
				SETFLOAT(&(pos_list[2*i+1]),x->mass[j].posY);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massPosL"),2*i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].posX);
		SETFLOAT(&(pos_list[1]),x->mass[(int)atom_getfloatarg(0, argc, argv)].posY);
			
        outlet_anything(x->main_outlet, gensym("massPosL"),2 , pos_list);        
    }
}

void pmpd2d_massForceL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[2*x->nb_mass];
 
    if (argc==0) 
    {
        for (i=0; i< x->nb_mass; i++) {
        SETFLOAT(&(pos_list[2*i]  ),x->mass[i].forceX);
        SETFLOAT(&(pos_list[2*i+1]),x->mass[i].forceY);
        }
        outlet_anything(x->main_outlet, gensym("massForceL"),2*x->nb_mass , pos_list);          
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
            {
				SETFLOAT(&(pos_list[2*i]  ),x->mass[j].forceX);
				SETFLOAT(&(pos_list[2*i+1]),x->mass[j].forceY);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massForceL"),2*i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].forceX);
        SETFLOAT(&(pos_list[1]),x->mass[(int)atom_getfloatarg(0, argc, argv)].forceY);
        
        outlet_anything(x->main_outlet, gensym("massForceL"),2 , pos_list);        
    }
}

void pmpd2d_massSpeedL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[2*x->nb_mass];
	
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++) {
			SETFLOAT(&(pos_list[2*i]  ),x->mass[i].speedX);
			SETFLOAT(&(pos_list[2*i+1]),x->mass[i].speedY);
		}
		outlet_anything(x->main_outlet, gensym("massSpeedL"),2*x->nb_mass , pos_list);         
    }
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
        i = 0;
        j = 0;
        while  (j < x->nb_mass)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[j].Id) 
			    {
				SETFLOAT(&(pos_list[2*i]  ),x->mass[j].speedX);
				SETFLOAT(&(pos_list[2*i+1]),x->mass[j].speedY);
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massSpeedL"),2*i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
        SETFLOAT(&(pos_list[0]),x->mass[(int)atom_getfloatarg(0, argc, argv)].speedX);
        SETFLOAT(&(pos_list[1]),x->mass[(int)atom_getfloatarg(0, argc, argv)].speedY);
        
        outlet_anything(x->main_outlet, gensym("massSpeedL"),2 , pos_list);        
    }
}

void pmpd2d_massPosXL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_massForceXL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_massSpeedXL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_massPosYL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_massForceYL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_massSpeedYL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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
// ---------------------------------------------------------------------

void pmpd2d_massPosNormL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++) {
			SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].posX)+sqr(x->mass[i].posY)));
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
				SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[j].posX)+sqr(x->mass[j].posY)));
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massPosNormL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
		i=(int)atom_getfloatarg(0, argc, argv);
		SETFLOAT(&(pos_list[0]),sqrt(sqr(x->mass[i].posX)+sqr(x->mass[i].posY)));
        outlet_anything(x->main_outlet, gensym("massPosNormL"),1 , pos_list);        
    }
}

void pmpd2d_massForceNormL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{   
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++) 
		{
			SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].forceX)+sqr(x->mass[i].forceY)));
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
				SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[j].forceX)+sqr(x->mass[j].forceY)));
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massForceNormL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
		i=(int)atom_getfloatarg(0, argc, argv);
        SETFLOAT(&(pos_list[0]),sqrt(sqr(x->mass[i].forceX)+sqr(x->mass[i].forceY)));
        outlet_anything(x->main_outlet, gensym("massForceNormL"),1 , pos_list);        
    }
}

void pmpd2d_massSpeedNormL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_mass];
    
    if (argc==0) 
    {
		for (i=0; i< x->nb_mass; i++) 
		{
			SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].speedX)+sqr(x->mass[i].speedY)));
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
				SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[j].speedX)+sqr(x->mass[j].speedY)));
                i++;
            }
            j++;
        }
        outlet_anything(x->main_outlet, gensym("massSpeedNormL"),i , pos_list);
    }
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
		i=(int)atom_getfloatarg(0, argc, argv);
        SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].speedX)+sqr(x->mass[i].speedY)));
        outlet_anything(x->main_outlet, gensym("massSpeedNormL"),1 , pos_list);        
    }
}

// ---------------------------------------------------------------------

void pmpd2d_linkPosL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[2*x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[2*i]  ),(x->link[i].mass2->posX + x->link[i].mass1->posX)/2);
			SETFLOAT(&(pos_list[2*i+1]),(x->link[i].mass2->posY + x->link[i].mass1->posY)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosL"),2*x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[2*j]  ),(x->link[i].mass2->posX + x->link[i].mass1->posX)/2);
				SETFLOAT(&(pos_list[2*j+1]),(x->link[i].mass2->posY + x->link[i].mass1->posY)/2);
				j++;
            }
        }
  		outlet_anything(x->main_outlet, gensym("linkPosL"), 2*j, pos_list);   
    }
}

void pmpd2d_linkLengthL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[2*x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[2*i]  ),x->link[i].mass2->posX - x->link[i].mass1->posX);
			SETFLOAT(&(pos_list[2*i+1]),x->link[i].mass2->posY - x->link[i].mass1->posY);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthL"),2*x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[2*j]  ),x->link[i].mass2->posX - x->link[i].mass1->posX);
				SETFLOAT(&(pos_list[2*j+1]),x->link[i].mass2->posY - x->link[i].mass1->posY);
				j++;
            }
        }
  		outlet_anything(x->main_outlet, gensym("linkLengthL"), 2*j, pos_list);   
    }
}

void pmpd2d_linkPosSpeedL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[2*x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[2*i]  ),(x->link[i].mass2->speedX + x->link[i].mass1->speedX)/2);
			SETFLOAT(&(pos_list[2*i+1]),(x->link[i].mass2->speedY + x->link[i].mass1->speedY)/2);
		}
		outlet_anything(x->main_outlet, gensym("linkPosSpeedL"),2*x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[2*j]  ),(x->link[i].mass2->speedX + x->link[i].mass1->speedX)/2);
				SETFLOAT(&(pos_list[2*j+1]),(x->link[i].mass2->speedY + x->link[i].mass1->speedY)/2);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosSpeedL"),2*j , pos_list);
    }
}

void pmpd2d_linkLengthSpeedL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[2*x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[2*i]  ),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
			SETFLOAT(&(pos_list[2*i+1]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
		}
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedL"),2*x->nb_link , pos_list);
	}
	else 
	if ( (argc==1) && (argv[0].a_type == A_SYMBOL) )
	{
		j=0;
		for (i=0; i < x->nb_link; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            {
				SETFLOAT(&(pos_list[2*j]  ),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
				SETFLOAT(&(pos_list[2*j+1]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedL"),2*j , pos_list);
    }
}

void pmpd2d_linkPosXL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_linkLengthXL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_linkPosSpeedXL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_linkLengthSpeedXL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_linkPosYL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_linkLengthYL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_linkPosSpeedYL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

void pmpd2d_linkLengthSpeedYL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

// ---------------------------------------------------------------------

void pmpd2d_linkPosNormL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + \
                            sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) ));
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
                            sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) ));
				j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosNormL"), j, pos_list);
    }		
}

void pmpd2d_linkLengthNormL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr(x->link[i].mass2->posX - x->link[i].mass1->posX) + \
                            sqr(x->link[i].mass2->posY - x->link[i].mass1->posY) ));
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
                            sqr(x->link[i].mass2->posY - x->link[i].mass1->posY) ));

				j++;
			}
        }
		outlet_anything(x->main_outlet, gensym("linkLengthNormL"), j, pos_list);
    }
}

void pmpd2d_linkPosSpeedNormL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2) + \
                            sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) ));
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
                            sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) ));
                j++;
            }
        }
		outlet_anything(x->main_outlet, gensym("linkPosSpeedNormL"), j, pos_list);
    }
}

// ---------------------------------------------------------------------

void pmpd2d_linkLengthSpeedNormL(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i,j;
    t_atom pos_list[x->nb_link];

    if (argc==0)
    {
		for (i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr(x->link[i].mass2->speedX - x->link[i].mass1->speedX) + \
                            sqr(x->link[i].mass2->speedY - x->link[i].mass1->speedY) ));
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
                            sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) ));
				j++;
			}
        }
		outlet_anything(x->main_outlet, gensym("linkLengthSpeedNormL"), j, pos_list);
    }
}

