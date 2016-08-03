void pmpd3d_massesPosL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[3*x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[3*i]  ),x->mass[i].posX);
        SETFLOAT(&(pos_list[3*i+1]),x->mass[i].posY);
        SETFLOAT(&(pos_list[3*i+2]),x->mass[i].posZ);
    }
    outlet_anything(x->main_outlet, gensym("massesPosL"),3*x->nb_mass , pos_list);
}

void pmpd3d_massesForcesL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[3*x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[3*i]  ),x->mass[i].forceX);
        SETFLOAT(&(pos_list[3*i+1]),x->mass[i].forceY);
        SETFLOAT(&(pos_list[3*i+2]),x->mass[i].forceZ);
    }
    outlet_anything(x->main_outlet, gensym("massesForcesL"),3*x->nb_mass , pos_list);
}

void pmpd3d_massesSpeedsL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[3*x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[3*i]  ),x->mass[i].speedX);
        SETFLOAT(&(pos_list[3*i+1]),x->mass[i].speedY);
        SETFLOAT(&(pos_list[3*i+2]),x->mass[i].speedZ);
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsL"),3*x->nb_mass , pos_list);
}

void pmpd3d_massesPosXL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].posX);
    }
    outlet_anything(x->main_outlet, gensym("massesPosXL"),x->nb_mass , pos_list);
}

void pmpd3d_massesForcesXL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].forceX);
    }
    outlet_anything(x->main_outlet, gensym("massesForcesXL"),x->nb_mass , pos_list);
}

void pmpd3d_massesSpeedsXL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].speedX);
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsXL"),x->nb_mass , pos_list);
}
void pmpd3d_massesPosYL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].posY);
    }
    outlet_anything(x->main_outlet, gensym("massesPosYL"),x->nb_mass , pos_list);
}

void pmpd3d_massesForcesYL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].forceY);
    }
    outlet_anything(x->main_outlet, gensym("massesForcesYL"),x->nb_mass , pos_list);
}

void pmpd3d_massesSpeedsYL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].speedY);
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsYL"),x->nb_mass , pos_list);
}

void pmpd3d_massesPosZL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].posZ);
    }
    outlet_anything(x->main_outlet, gensym("massesPosZL"),x->nb_mass , pos_list);
}

void pmpd3d_massesForcesZL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].forceZ);
    }
    outlet_anything(x->main_outlet, gensym("massesForcesZL"),x->nb_mass , pos_list);
}

void pmpd3d_massesSpeedsZL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].speedZ);
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsZL"),x->nb_mass , pos_list);
}


void pmpd3d_massesPosNormL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].posX)+sqr(x->mass[i].posY)+sqr(x->mass[i].posZ)));
    }
    outlet_anything(x->main_outlet, gensym("massesPosNormL"),x->nb_mass , pos_list);
}

void pmpd3d_massesForcesNormL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].forceX)+sqr(x->mass[i].forceY)+sqr(x->mass[i].forceZ)));
    }
    outlet_anything(x->main_outlet, gensym("massesForcesNormL"),x->nb_mass , pos_list);
}

void pmpd3d_massesSpeedsNormL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].speedX)+sqr(x->mass[i].speedY)+sqr(x->mass[i].speedZ)));
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsNormL"),x->nb_mass , pos_list);
}


void pmpd3d_massesPosMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, sommeZ, somme;
    t_int i,j;
    t_atom mean[4];

	sommeX = 0;
	sommeY = 0;
	sommeZ = 0;
	somme = 0;
	j = 0;
	
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				sommeX += x->mass[i].posX;
				sommeY += x->mass[i].posY;
				sommeZ += x->mass[i].posZ;
				somme +=  sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY) + sqr(x->mass[i].posZ)); // distance au centre
				j++;
			}
		}
    }
	else
	{
		for (i=0; i< x->nb_mass; i++)
        {
				sommeX += x->mass[i].posX;
				sommeY += x->mass[i].posY;
				sommeZ += x->mass[i].posZ;
				somme +=  sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY) + sqr(x->mass[i].posZ)); // distance au centre
				j++;
		}
	}	
	
	sommeX /= j;
	sommeY /= j;
	sommeZ /= j;
	somme  /= j;	
	
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("massesPosMean"),4 , mean);
}

void pmpd3d_massesPosStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, sommeZ, somme;
    t_int i,j;
    t_float stdX, stdY, stdZ, std;
    t_atom std_out[4];

	sommeX = 0;
	sommeY = 0;
	sommeZ = 0;
	somme  = 0;
	stdX = 0;
	stdY = 0;
	stdZ = 0;
	std  = 0;
	j = 0;
	
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				sommeX += x->mass[i].posX;
				sommeY += x->mass[i].posY;
				sommeZ += x->mass[i].posZ;
				somme +=  sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY) + sqr(x->mass[i].posZ)); // distance au centre
				j++;
			}
		}
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme /= j;
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				stdX += sqr(x->mass[i].posX-sommeX);
				stdY += sqr(x->mass[i].posY-sommeY);
				stdZ += sqr(x->mass[i].posZ-sommeZ);
				std  +=  sqr(sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY) + sqr(x->mass[i].posZ))-somme);
			}
		}		
    }
	else
	{
		for (i=0; i< x->nb_mass; i++)
        {
			sommeX += x->mass[i].posX;
			sommeY += x->mass[i].posY;
			sommeZ += x->mass[i].posZ;
			somme +=  sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY) + sqr(x->mass[i].posZ)); // distance au centre
			j++;
		}
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme /= j;
		for (i=0; i< x->nb_mass; i++)
        {
			stdX += sqr(x->mass[i].posX-sommeX);
			stdY += sqr(x->mass[i].posY-sommeY);
			stdZ += sqr(x->mass[i].posZ-sommeZ);
			std  += sqr(sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY) + sqr(x->mass[i].posZ))-somme);
		}
	}	
	
	stdX = sqrt(stdX/j);
	stdY = sqrt(stdY/j);
	stdZ = sqrt(stdZ/j);
	std  = sqrt(std /j);	

    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),stdZ);
    SETFLOAT(&(std_out[3]),std);
    
    outlet_anything(x->main_outlet, gensym("massesPosStd"),4 , std_out);
}

void pmpd3d_massesForcesMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, sommeZ, somme;
    t_int i,j;
    t_atom mean[4];

	sommeX = 0;
	sommeY = 0;
	sommeZ = 0;
	somme = 0;
	j = 0;
	
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				sommeX += x->mass[i].forceX;
				sommeY += x->mass[i].forceY;
				sommeZ += x->mass[i].forceZ;
				somme +=  sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY) + sqr(x->mass[i].forceZ)); // force total
				j++;
			}
		}
    }
	else
	{
		for (i=0; i< x->nb_mass; i++)
        {
				sommeX += x->mass[i].forceX;
				sommeY += x->mass[i].forceY;
				sommeZ += x->mass[i].forceZ;
				somme +=  sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY) + sqr(x->mass[i].forceZ)); // force
				j++;
		}
	}	
	
	sommeX /= j;
	sommeY /= j;
	sommeZ /= j;
	somme  /= j;

    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("massesForcesMean"),4 , mean);
}

void pmpd3d_massesForcesStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, sommeZ, somme;
    t_int i,j;
    t_float stdX, stdY, stdZ, std;
    t_atom std_out[4];

	sommeX = 0;
	sommeY = 0;
	sommeZ = 0;
	somme  = 0;
	stdX = 0;
	stdY = 0;
	stdZ = 0;
	std  = 0;
	j = 0;
	
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				sommeX += x->mass[i].forceX;
				sommeY += x->mass[i].forceY;
				sommeZ += x->mass[i].forceZ;
				somme +=  sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY) + sqr(x->mass[i].forceZ)); // force
				j++;
			}
		}
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme /= j;
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				stdX += sqr(x->mass[i].forceX-sommeX);
				stdY += sqr(x->mass[i].forceY-sommeY);
				stdZ += sqr(x->mass[i].forceZ-sommeZ);
				std  +=  sqr(sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY) + sqr(x->mass[i].forceZ))-somme);
			}
		}		
    }
	else
	{
		for (i=0; i< x->nb_mass; i++)
        {
			sommeX += x->mass[i].forceX;
			sommeY += x->mass[i].forceY;
			sommeZ += x->mass[i].forceZ;
			somme +=  sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY) + sqr(x->mass[i].forceZ)); 
			j++;
		}
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme /= j;
		for (i=0; i< x->nb_mass; i++)
        {
			stdX += sqr(x->mass[i].forceX-sommeX);
			stdY += sqr(x->mass[i].forceY-sommeY);
			stdZ += sqr(x->mass[i].forceZ-sommeZ);
			std  += sqr(sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY) + sqr(x->mass[i].forceZ))-somme);
		}
	}	
	
	stdX = sqrt(stdX/j);
	stdY = sqrt(stdY/j);
	stdZ = sqrt(stdZ/j);
	std  = sqrt(std /j);	

    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),stdZ);
    SETFLOAT(&(std_out[3]),std);
    
    outlet_anything(x->main_outlet, gensym("massesForcesStd"),4 , std_out);
}

void pmpd3d_massesSpeedsMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, sommeZ, somme;
    t_int i,j;
    t_atom mean[4];

	sommeX = 0;
	sommeY = 0;
	sommeZ = 0;
	somme = 0;
	j = 0;
	
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				sommeX += x->mass[i].speedX;
				sommeY += x->mass[i].speedY;
				sommeZ += x->mass[i].speedZ;
				somme +=  sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY) + sqr(x->mass[i].speedZ)); // speed total
				j++;
			}
		}
    }
	else
	{
		for (i=0; i< x->nb_mass; i++)
        {
				sommeX += x->mass[i].speedX;
				sommeY += x->mass[i].speedY;
				sommeZ += x->mass[i].speedZ;
				somme +=  sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY) + sqr(x->mass[i].speedZ)); // speed
				j++;
		}
	}	
	
	sommeX /= j;
	sommeY /= j;
	sommeZ /= j;
	somme  /= j;

    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("massesFpeedsMean"),4 , mean);
}

void pmpd3d_massesSpeedsStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, sommeZ, somme;
    t_int i,j;
    t_float stdX, stdY, stdZ, std;
    t_atom std_out[4];

	sommeX = 0;
	sommeY = 0;
	sommeZ = 0;
	somme  = 0;
	stdX = 0;
	stdY = 0;
	stdZ = 0;
	std  = 0;
	j = 0;
	
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				sommeX += x->mass[i].speedX;
				sommeY += x->mass[i].speedY;
				sommeZ += x->mass[i].speedZ;
				somme +=  sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY) + sqr(x->mass[i].speedZ)); // speed
				j++;
			}
		}
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme /= j;
		for (i=0; i< x->nb_mass; i++)
        {
			if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
			{ 
				stdX += sqr(x->mass[i].speedX-sommeX);
				stdY += sqr(x->mass[i].speedY-sommeY);
				stdZ += sqr(x->mass[i].speedZ-sommeZ);
				std  +=  sqr(sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY) + sqr(x->mass[i].speedZ))-somme);
			}
		}		
    }
	else
	{
		for (i=0; i< x->nb_mass; i++)
        {
			sommeX += x->mass[i].speedX;
			sommeY += x->mass[i].speedY;
			sommeZ += x->mass[i].speedZ;
			somme +=  sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY) + sqr(x->mass[i].speedZ)); 
			j++;
		}
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme /= j;
		for (i=0; i< x->nb_mass; i++)
        {
			stdX += sqr(x->mass[i].speedX-sommeX);
			stdY += sqr(x->mass[i].speedY-sommeY);
			stdZ += sqr(x->mass[i].speedZ-sommeZ);
			std  += sqr(sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY) + sqr(x->mass[i].speedZ))-somme);
		}
	}	
	
	stdX = sqrt(stdX/j);
	stdY = sqrt(stdY/j);
	stdZ = sqrt(stdZ/j);
	std  = sqrt(std /j);	

    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),stdZ);
    SETFLOAT(&(std_out[3]),std);
    
    outlet_anything(x->main_outlet, gensym("massesSpeedsStd"),4 , std_out);
}

// --------------------------------------------

void pmpd3d_linksPosL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[3*x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[3*i]  ),(x->link[i].mass2->posX + x->link[i].mass1->posX)/2);
        SETFLOAT(&(pos_list[3*i+1]),(x->link[i].mass2->posY + x->link[i].mass1->posY)/2);
        SETFLOAT(&(pos_list[3*i+2]),(x->link[i].mass2->posZ + x->link[i].mass1->posZ)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosL"),3*x->nb_link , pos_list);
}

void pmpd3d_linksLengthL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[3*x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[3*i]  ),x->link[i].mass2->posX - x->link[i].mass1->posX);
        SETFLOAT(&(pos_list[3*i+1]),x->link[i].mass2->posY - x->link[i].mass1->posY);
        SETFLOAT(&(pos_list[3*i+2]),x->link[i].mass2->posZ - x->link[i].mass1->posZ);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthL"),3*x->nb_link , pos_list);
}

void pmpd3d_linksPosSpeedL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[3*x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[3*i]  ),(x->link[i].mass2->speedX + x->link[i].mass1->speedX)/2);
        SETFLOAT(&(pos_list[3*i+1]),(x->link[i].mass2->speedY + x->link[i].mass1->speedY)/2);
        SETFLOAT(&(pos_list[3*i+2]),(x->link[i].mass2->speedZ + x->link[i].mass1->speedZ)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedL"),3*x->nb_link , pos_list);
}

void pmpd3d_linksLengthSpeedL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[3*x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[3*i]  ),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
        SETFLOAT(&(pos_list[3*i+1]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
        SETFLOAT(&(pos_list[3*i+2]),x->link[i].mass2->speedZ - x->link[i].mass1->speedZ);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedL"),3*x->nb_link , pos_list);
}

void pmpd3d_linksPosXL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->posX + x->link[i].mass2->posX)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosXL"),x->nb_link , pos_list);
}

void pmpd3d_linksLengthXL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->posX - x->link[i].mass1->posX);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthXL"),x->nb_link , pos_list);
}

void pmpd3d_linksPosSpeedXL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedXL"),x->nb_link , pos_list);
}

void pmpd3d_linksLengthSpeedXL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedXL"),x->nb_link , pos_list);
}

void pmpd3d_linksPosYL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->posY + x->link[i].mass2->posY)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosYL"),x->nb_link , pos_list);
}

void pmpd3d_linksLengthYL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->posY - x->link[i].mass1->posY);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthYL"),x->nb_link , pos_list);
}

void pmpd3d_linksPosSpeedYL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedYL"),x->nb_link , pos_list);
}

void pmpd3d_linksLengthSpeedYL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedYL"),x->nb_link , pos_list);
}

void pmpd3d_linksPosZL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosZL"),x->nb_link , pos_list);
}

void pmpd3d_linksLengthZL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->posZ - x->link[i].mass1->posZ);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthZL"),x->nb_link , pos_list);
}

void pmpd3d_linksPosSpeedZL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedZL"),x->nb_link , pos_list);
}

void pmpd3d_linksLengthSpeedZL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->speedZ - x->link[i].mass1->speedZ);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedZL"),x->nb_link , pos_list);
}


void pmpd3d_linksPosNormL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt( \
							sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + \
							sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + \
							sqr((x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2) ));
    }
    outlet_anything(x->main_outlet, gensym("linksPosNormL"),x->nb_link , pos_list);
}

void pmpd3d_linksLengthNormL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt( \
							sqr(x->link[i].mass2->posX - x->link[i].mass1->posX) + \
							sqr(x->link[i].mass2->posY - x->link[i].mass1->posY) + \
							sqr(x->link[i].mass2->posZ - x->link[i].mass1->posZ) ));
    }
    outlet_anything(x->main_outlet, gensym("linksLengthNormL"),x->nb_link , pos_list);
}

void pmpd3d_linksPosSpeedNormL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt( \
							sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2) + \
							sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + \
							sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2) ));
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedNormL"),x->nb_link , pos_list);
}

void pmpd3d_linksLengthSpeedNormL(t_pmpd3d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt( \
							sqr(x->link[i].mass2->speedX - x->link[i].mass1->speedX) + \
							sqr(x->link[i].mass2->speedY - x->link[i].mass1->speedY) + \
							sqr(x->link[i].mass2->speedZ - x->link[i].mass1->speedZ) ));
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedNormL"),x->nb_link , pos_list);
}
