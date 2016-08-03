void pmpd3d_massPosMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
    
    if ( j> 0)
	{
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }    
    
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("massPosMean"),4 , mean);
}

void pmpd3d_massPosStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
		if ( j> 0)
		{
			sommeX /= j;
			sommeY /= j;
			sommeZ /= j;
			somme  /= j;    
		}
    
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
		if ( j> 0)
		{
			sommeX /= j;
			sommeY /= j;
			sommeZ /= j;
			somme  /= j;    
		}
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
    
    outlet_anything(x->main_outlet, gensym("massPosStd"),4 , std_out);
}

void pmpd3d_massForceMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
    
    if ( j> 0)
	{
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }

    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("massForcesMean"),4 , mean);
}

void pmpd3d_massForceStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
	    if ( j> 0)
		{
			sommeX /= j;
			sommeY /= j;
			sommeZ /= j;
			somme  /= j;    
	    }
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
	    if ( j> 0)
		{
			sommeX /= j;
			sommeY /= j;
			sommeZ /= j;
			somme  /= j;    
	    }
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
    
    outlet_anything(x->main_outlet, gensym("massForcesStd"),4 , std_out);
}

void pmpd3d_massSpeedMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
    
    if ( j> 0)
	{
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }

    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("massSpeedMean"),4 , mean);
}

void pmpd3d_massSpeedStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
	    if ( j> 0)
		{
			sommeX /= j;
			sommeY /= j;
			sommeZ /= j;
			somme  /= j;    
	    }
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
	    if ( j> 0)
		{
			sommeX /= j;
			sommeY /= j;
			sommeZ /= j;
			somme  /= j;    
	    }
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
    
    outlet_anything(x->main_outlet, gensym("massSpeedStd"),4 , std_out);
}

void pmpd3d_linkPosMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                sommeX += (x->link[i].mass1->posX + x->link[i].mass2->posX)/2;
                sommeY += (x->link[i].mass1->posY + x->link[i].mass2->posY)/2;
                sommeZ += (x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2;
                somme  += sqrt(sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + sqr((x->link[i].mass1->posZ + x->link[i].mass1->posZ)/2));
                j++;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
                sommeX += (x->link[i].mass1->posX + x->link[i].mass2->posX)/2;
                sommeY += (x->link[i].mass1->posY + x->link[i].mass2->posY)/2;
                sommeZ += (x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2;
                somme  += sqrt(sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + sqr((x->link[i].mass1->posZ + x->link[i].mass1->posZ)/2));
                j++;
        }
    }    
    
    if ( j> 0)
	{
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }
    
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("linkPosMean"),4 , mean);
}

void pmpd3d_linkLengthMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                sommeX += fabs(x->link[i].mass1->posX - x->link[i].mass2->posX);
                sommeY += fabs(x->link[i].mass1->posY - x->link[i].mass2->posY);
                sommeZ += fabs(x->link[i].mass1->posZ - x->link[i].mass2->posZ);
                somme  += x->link[i].distance;
                j+=1;
            }
        }
    }
    else if (argc == 0)
    {
        for (i=0; i< x->nb_link; i++)
        {
            sommeX += fabs(x->link[i].mass1->posX - x->link[i].mass2->posX);
            sommeY += fabs(x->link[i].mass1->posY - x->link[i].mass2->posY);
            sommeZ += fabs(x->link[i].mass1->posZ - x->link[i].mass2->posZ);
            somme  += x->link[i].distance;
            j+=1;
        }
    }    
    
    if (j>0)
    {
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }
    
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("linkLengthMean"),4 , mean);
}

void pmpd3d_linkPosSpeedMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                sommeX += (x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2;
                sommeY += (x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2;
                sommeZ += (x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2;
                somme  += sqrt(sqr((x->link[i].mass1->speedX+x->link[i].mass2->speedX)/2) + sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2));
                j++;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
            sommeX += (x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2;
            sommeY += (x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2;
            sommeZ += (x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2;
            somme  += sqrt(sqr((x->link[i].mass1->speedX+x->link[i].mass2->speedX)/2) + sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2));
            j++;
        }
    }    
    
    if (j>0)
    {
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }   
    
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("linkPosSpeedMean"),4 , mean);
}

void pmpd3d_linkLengthSpeedMean(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
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
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                sommeX += fabs(x->link[i].mass1->speedX - x->link[i].mass2->speedX);
                sommeY += fabs(x->link[i].mass1->speedY - x->link[i].mass2->speedY);
                sommeZ += fabs(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ);
                somme  += sqrt(sqr(x->link[i].mass1->speedX - x->link[i].mass2->speedX) +  
							sqr(x->link[i].mass1->speedY - x->link[i].mass2->speedY) + 
							sqr(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ));
				j++;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
			sommeX += fabs(x->link[i].mass1->speedX - x->link[i].mass2->speedX);
			sommeY += fabs(x->link[i].mass1->speedY - x->link[i].mass2->speedY);
			sommeZ += fabs(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ);
			somme  += sqrt(sqr(x->link[i].mass1->speedX - x->link[i].mass2->speedX) +  
						sqr(x->link[i].mass1->speedY - x->link[i].mass2->speedY) + 
						sqr(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ));
			j++;
        }
    }    
    
    if (j>0)
    {
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }    
    
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),sommeZ);
    SETFLOAT(&(mean[3]),somme);
    
    outlet_anything(x->main_outlet, gensym("linkLengthSpeedMean"),4 , mean);
}

void pmpd3d_linkPosStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, sommeZ, somme;
    t_int i,j;
    t_float stdX, stdY, stdZ, std;
    t_atom std_out[4];

    sommeX = 0;
    sommeY = 0;
    sommeZ = 0;
    somme = 0;
    stdX = 0;
    stdY = 0;
    stdZ = 0;
    std  = 0;
    j = 0;
    
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                sommeX += (x->link[i].mass1->posX + x->link[i].mass2->posX)/2;
                sommeY += (x->link[i].mass1->posY + x->link[i].mass2->posY)/2;
                sommeZ += (x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2;
                somme  += sqrt(sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + sqr((x->link[i].mass1->posZ + x->link[i].mass1->posZ)/2));
                j++;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
            sommeX += (x->link[i].mass1->posX + x->link[i].mass2->posX)/2;
            sommeY += (x->link[i].mass1->posY + x->link[i].mass2->posY)/2;
            sommeZ += (x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2;
            somme  += sqrt(sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + sqr((x->link[i].mass1->posZ + x->link[i].mass1->posZ)/2));
            j++;
        }
    }    
    
    if (j>0)
    {
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }    
    
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                stdX += sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2 - sommeX);
                stdY += sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2 - sommeY);
                stdZ += sqr((x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2 - sommeZ);
                std  += sqr(sqrt(sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + sqr((x->link[i].mass1->posZ + x->link[i].mass1->posZ)/2)) - somme);
                j+=1;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
            stdX += sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2 - sommeX);
            stdY += sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2 - sommeY);
            stdZ += sqr((x->link[i].mass1->posZ + x->link[i].mass2->posZ)/2 - sommeZ);
            std  += sqr(sqrt(sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) + sqr((x->link[i].mass1->posZ + x->link[i].mass1->posZ)/2)) - somme);
            j+=1;
        }
    }        
    
    if ( j > 0)
    {
		stdX = sqrt(stdX/j);
		stdY = sqrt(stdY/j);
		stdZ = sqrt(stdZ/j);
		std  = sqrt(std /j);    
	}
	
    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),stdZ);
    SETFLOAT(&(std_out[3]),std);
    
    outlet_anything(x->main_outlet, gensym("linkPosStd"),4 , std_out);
}

void pmpd3d_linkLengthStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	t_float sommeX, sommeY, sommeZ, somme;
    t_int i,j;
    t_float stdX, stdY, stdZ, std;
    t_atom std_out[4];

    sommeX = 0;
    sommeY = 0;
    sommeZ = 0;
    somme = 0;
    stdX = 0;
    stdY = 0;
    stdZ = 0;
    std  = 0;
    j = 0;
    
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                sommeX += fabs(x->link[i].mass1->posX - x->link[i].mass2->posX);
                sommeY += fabs(x->link[i].mass1->posY - x->link[i].mass2->posY);
                sommeZ += fabs(x->link[i].mass1->posZ - x->link[i].mass2->posZ);
                somme  += x->link[i].distance;
                j+=1;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
            sommeX += fabs(x->link[i].mass1->posX - x->link[i].mass2->posX);
            sommeY += fabs(x->link[i].mass1->posY - x->link[i].mass2->posY);
            sommeZ += fabs(x->link[i].mass1->posZ - x->link[i].mass2->posZ);
            somme  += x->link[i].distance;
            j+=1;
        }
    }    
    
    if ( j> 0)
	{
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }
    
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                stdX += sqr(fabs(x->link[i].mass1->posX - x->link[i].mass2->posX)-sommeX);
                stdY += sqr(fabs(x->link[i].mass1->posY - x->link[i].mass2->posY)-sommeY);
                stdZ += sqr(fabs(x->link[i].mass1->posZ - x->link[i].mass2->posZ)-sommeZ);
                std  += sqr(x->link[i].distance - somme);
                j+=1;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
            stdX += sqr(fabs(x->link[i].mass1->posX - x->link[i].mass2->posX) - sommeX);
            stdY += sqr(fabs(x->link[i].mass1->posY - x->link[i].mass2->posY) - sommeY);
            stdZ += sqr(fabs(x->link[i].mass1->posZ - x->link[i].mass2->posZ) - sommeZ);
            std  += sqr(x->link[i].distance - somme);
            j+=1;
        }
    }   
     
    if ( j > 0)
    {
		stdX = sqrt(stdX/j);
		stdY = sqrt(stdY/j);
		stdZ = sqrt(stdZ/j);
		std  = sqrt(std /j);    
	}
  

    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),stdZ);
    SETFLOAT(&(std_out[3]),std);
    
    outlet_anything(x->main_outlet, gensym("linkLengthStd"),4 , std_out);
}

void pmpd3d_linkPosSpeedStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	t_float sommeX, sommeY, sommeZ, somme;
	t_float stdX, stdY, stdZ, std;
    t_int i,j;
    t_atom std_out[4];

    sommeX = 0;
    sommeY = 0;
    sommeZ = 0;
    somme = 0;
    stdX = 0;
    stdY = 0;
    stdZ = 0;
    std  = 0;
    j = 0;
    
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                sommeX += (x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2;
                sommeY += (x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2;
                sommeZ += (x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2;
                somme  += sqrt(sqr((x->link[i].mass1->speedX+x->link[i].mass2->speedX)/2) + sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2));
                j++;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
                sommeX += (x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2;
                sommeY += (x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2;
                sommeZ += (x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2;
                somme  += sqrt(sqr((x->link[i].mass1->speedX+x->link[i].mass2->speedX)/2) + sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2));
                j++;
        }
    }    
    
    if ( j> 0)
	{
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }  

    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                stdX += sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2-sommeX);
                stdY += sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2-sommeY);
                stdZ += sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2-sommeZ);
                std  +=  sqr(sqrt(sqr((x->link[i].mass1->speedX+x->link[i].mass2->speedX)/2) + sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2)) - somme);
                j++;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
            stdX += sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2-sommeX);
            stdY += sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2-sommeY);
            stdZ += sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2-sommeZ);
            std  +=  sqr(sqrt(sqr((x->link[i].mass1->speedX+x->link[i].mass2->speedX)/2) + sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2) + sqr((x->link[i].mass1->speedZ + x->link[i].mass2->speedZ)/2)) - somme);
            j++;
        }
    }     
    
    if ( j > 0)
    {
		stdX = sqrt(stdX/j);
		stdY = sqrt(stdY/j);
		stdZ = sqrt(stdZ/j);
		std  = sqrt(std /j);    
	}
 

    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),stdZ);
    SETFLOAT(&(std_out[3]),std);
    
    outlet_anything(x->main_outlet, gensym("linkPosSpeedStd"),4 , std_out);
}

void pmpd3d_linkLengthSpeedStd(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	t_float sommeX, sommeY, sommeZ, somme;
    t_float stdX, stdY, stdZ, std;
    t_int i,j;
    t_atom std_out[4];

    sommeX = 0;
    sommeY = 0;
    sommeZ = 0;
    somme = 0;
    stdX = 0;
    stdY = 0;
    stdZ = 0;
    std  = 0;
    j = 0;
    
    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                sommeX += fabs(x->link[i].mass1->speedX - x->link[i].mass2->speedX);
                sommeY += fabs(x->link[i].mass1->speedY - x->link[i].mass2->speedY);
                sommeZ += fabs(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ);
                somme  += sqrt(sqr(x->link[i].mass1->speedX - x->link[i].mass2->speedX) +  
							sqr(x->link[i].mass1->speedY - x->link[i].mass2->speedY) + 
							sqr(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ));
				j++;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
			sommeX += fabs(x->link[i].mass1->speedX - x->link[i].mass2->speedX);
			sommeY += fabs(x->link[i].mass1->speedY - x->link[i].mass2->speedY);
			sommeZ += fabs(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ);
			somme  += sqrt(sqr(x->link[i].mass1->speedX - x->link[i].mass2->speedX) +  
						sqr(x->link[i].mass1->speedY - x->link[i].mass2->speedY) + 
						sqr(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ));
			j++;
        }
    }    
    
    if ( j> 0)
	{
		sommeX /= j;
		sommeY /= j;
		sommeZ /= j;
		somme  /= j;    
    }    

    if ( (argc >= 1) && (argv[0].a_type == A_SYMBOL) ) 
    {
        for (i=0; i< x->nb_link; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id)
            { 
                stdX += sqr(fabs(x->link[i].mass1->speedX - x->link[i].mass2->speedX) - sommeX);
                stdY += sqr(fabs(x->link[i].mass1->speedY - x->link[i].mass2->speedY) - sommeY);
                stdZ += sqr(fabs(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ) - sommeZ);
                std  += sqr( sqrt(sqr(x->link[i].mass1->speedX - x->link[i].mass2->speedX) +  
							sqr(x->link[i].mass1->speedY - x->link[i].mass2->speedY) + 
							sqr(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ)) - somme);
				j++;
            }
        }
    }
    else
    {
        for (i=0; i< x->nb_link; i++)
        {
            stdX += sqr(fabs(x->link[i].mass1->speedX - x->link[i].mass2->speedX) - sommeX);
            stdY += sqr(fabs(x->link[i].mass1->speedY - x->link[i].mass2->speedY) - sommeY);
            stdZ += sqr(fabs(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ) - sommeZ);
            std  += sqr( sqrt(sqr(x->link[i].mass1->speedX - x->link[i].mass2->speedX) +  
							sqr(x->link[i].mass1->speedY - x->link[i].mass2->speedY) + 
							sqr(x->link[i].mass1->speedZ - x->link[i].mass2->speedZ)) - somme);
			j++;
        }
    }

    if ( j > 0)
    {
		stdX = sqrt(stdX/j);
		stdY = sqrt(stdY/j);
		stdZ = sqrt(stdZ/j);
		std  = sqrt(std /j);    
	}
    
    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),stdZ);
    SETFLOAT(&(std_out[3]),std);
    
    outlet_anything(x->main_outlet, gensym("linkLengthSpeedStd"),4 , std_out);
}


void pmpd3d_massInfo(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	t_atom info[14];
	int i;
	
    if (argc==0) 
    {
		for(i=0; i < x->nb_mass; i++)
		{
			SETFLOAT(&(info[0]),  i);
			SETSYMBOL(&(info[1]), x->mass[i].Id);
			SETFLOAT(&(info[2]),  x->mass[i].mobile);
			SETFLOAT(&(info[3]),  1/x->mass[i].invM);
			SETFLOAT(&(info[4]),  x->mass[i].D2);
			SETFLOAT(&(info[5]),  x->mass[i].posX);
			SETFLOAT(&(info[6]),  x->mass[i].posY);
			SETFLOAT(&(info[7]),  x->mass[i].posZ);
			SETFLOAT(&(info[8]),  x->mass[i].speedX);
			SETFLOAT(&(info[9]),  x->mass[i].speedY);
			SETFLOAT(&(info[10]), x->mass[i].speedZ);
			SETFLOAT(&(info[11]), x->mass[i].forceX);
			SETFLOAT(&(info[12]), x->mass[i].forceY);
			SETFLOAT(&(info[13]), x->mass[i].forceZ);
			outlet_anything(x->main_outlet, gensym("massInfo"), 14, info);
		}		
	}
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
		for(i=0; i < x->nb_mass; i++)
		{
            if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id) 
			{
				SETFLOAT(&(info[0]),  i);
				SETSYMBOL(&(info[1]), x->mass[i].Id);
				SETFLOAT(&(info[2]),  x->mass[i].mobile);
				SETFLOAT(&(info[3]),  1/x->mass[i].invM);
				SETFLOAT(&(info[4]),  x->mass[i].D2);
				SETFLOAT(&(info[5]),  x->mass[i].posX);
				SETFLOAT(&(info[6]),  x->mass[i].posY);
				SETFLOAT(&(info[7]),  x->mass[i].posZ);
				SETFLOAT(&(info[8]),  x->mass[i].speedX);
				SETFLOAT(&(info[9]),  x->mass[i].speedY);
				SETFLOAT(&(info[10]), x->mass[i].speedZ);
				SETFLOAT(&(info[11]), x->mass[i].forceX);
				SETFLOAT(&(info[12]), x->mass[i].forceY);
				SETFLOAT(&(info[13]), x->mass[i].forceZ);
				outlet_anything(x->main_outlet, gensym("massInfo"), 14, info);
			}
		}
	}
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
		i=(int)atom_getfloatarg(0, argc, argv);
		i=max(i,0);
		i=min(i,x->nb_mass-1);
		
		SETFLOAT(&(info[0]),  i);
		SETSYMBOL(&(info[1]), x->mass[i].Id);
		SETFLOAT(&(info[2]),  x->mass[i].mobile);
		SETFLOAT(&(info[3]),  1/x->mass[i].invM);
		SETFLOAT(&(info[4]),  x->mass[i].D2);
		SETFLOAT(&(info[5]),  x->mass[i].posX);
		SETFLOAT(&(info[6]),  x->mass[i].posY);
		SETFLOAT(&(info[7]),  x->mass[i].posZ);
		SETFLOAT(&(info[8]),  x->mass[i].speedX);
		SETFLOAT(&(info[9]),  x->mass[i].speedY);
		SETFLOAT(&(info[10]), x->mass[i].speedZ);
		SETFLOAT(&(info[11]), x->mass[i].forceX);
		SETFLOAT(&(info[12]), x->mass[i].forceY);
		SETFLOAT(&(info[13]), x->mass[i].forceZ);
		outlet_anything(x->main_outlet, gensym("massInfo"), 14, info);
	}
}

void pmpd3d_linkInfo(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	t_atom info[14];
	int i, k;
	
    if (argc==0) 
    {
		for(i=0; i < x->nb_link; i++)
		{
			SETFLOAT(&(info[1]),  i);
			SETSYMBOL(&(info[2]), x->link[i].Id);
			SETFLOAT(&(info[3]),  x->link[i].active);
			SETFLOAT(&(info[4]),  x->link[i].mass1->num);
			SETFLOAT(&(info[5]),  x->link[i].mass2->num);
			SETFLOAT(&(info[6]),  x->link[i].K);
			SETFLOAT(&(info[7]),  x->link[i].D);
			
			switch(x->link[i].lType)
			{
			case 0 :
				SETSYMBOL(&(info[0]),  gensym("link"));
				SETFLOAT(&(info[8]),   x->link[i].Pow);
				SETFLOAT(&(info[9]),   x->link[i].L);
				SETFLOAT(&(info[10]),   x->link[i].Lmin);
				SETFLOAT(&(info[11]),  x->link[i].Lmax);
				outlet_anything(x->main_outlet, gensym("linkInfo"), 12, info);			
				break;
			case 1 :
				SETSYMBOL(&(info[0]), gensym("tLink"));
				SETFLOAT(&(info[8]),   x->link[i].Pow);
				SETFLOAT(&(info[9]),   x->link[i].L);
				SETFLOAT(&(info[10]),   x->link[i].Lmin);
				SETFLOAT(&(info[11]),  x->link[i].Lmax);
				SETFLOAT(&(info[12]),  x->link[i].VX);
				SETFLOAT(&(info[13]),  x->link[i].VY);				
				outlet_anything(x->main_outlet, gensym("linkInfo"), 14, info);	
				break;
			case 2 :
				SETSYMBOL(&(info[0]), gensym("tabLink"));
				SETSYMBOL(&(info[8]), x->link[i].arrayK);
				SETFLOAT(&(info[9]),  x->link[i].K_L);
				SETSYMBOL(&(info[10]), x->link[i].arrayD);
				SETFLOAT(&(info[11]), x->link[i].D_L);
				outlet_anything(x->main_outlet, gensym("linkInfo"), 12, info);	
			}
		}		
	}
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
		for(i=0; i < x->nb_link; i++)
		{
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id) 
			{
				SETFLOAT(&(info[1]),  i);
				SETSYMBOL(&(info[2]), x->link[i].Id);
				SETFLOAT(&(info[3]),  x->link[i].active);
				SETFLOAT(&(info[4]),  x->link[i].mass1->num);
				SETFLOAT(&(info[5]),  x->link[i].mass2->num);
				SETFLOAT(&(info[6]),  x->link[i].K);
				SETFLOAT(&(info[7]),  x->link[i].D);
			
				switch(x->link[i].lType)
				{
				case 0 :
					SETSYMBOL(&(info[0]),  gensym("link"));
					SETFLOAT(&(info[8]),   x->link[i].Pow);
					SETFLOAT(&(info[9]),   x->link[i].L);
					SETFLOAT(&(info[10]),   x->link[i].Lmin);
					SETFLOAT(&(info[11]),  x->link[i].Lmax);
					outlet_anything(x->main_outlet, gensym("linkInfo"), 12, info);		
					break;
				case 1 :
					SETSYMBOL(&(info[0]), gensym("tLink"));
					SETFLOAT(&(info[8]),   x->link[i].Pow);
					SETFLOAT(&(info[9]),   x->link[i].L);
					SETFLOAT(&(info[10]),   x->link[i].Lmin);
					SETFLOAT(&(info[11]),  x->link[i].Lmax);
					SETFLOAT(&(info[12]),  x->link[i].VX);
					SETFLOAT(&(info[13]),  x->link[i].VY);				
					outlet_anything(x->main_outlet, gensym("linkInfo"), 14, info);		
					break;
				case 2 :
					SETSYMBOL(&(info[0]), gensym("tabLink"));
					SETSYMBOL(&(info[8]), x->link[i].arrayK);
					SETFLOAT(&(info[9]),  x->link[i].K_L);
					SETSYMBOL(&(info[10]), x->link[i].arrayD);
					SETFLOAT(&(info[11]), x->link[i].D_L);
					outlet_anything(x->main_outlet, gensym("linkInfo"), 12, info);	
				}
			}	
		}
	}
    else if ((argc==1) && (argv[0].a_type == A_FLOAT)) 
    {
		i=(int)atom_getfloatarg(0, argc, argv);
		i=max(i,0);
		i=min(i,x->nb_link-1);
		
		SETFLOAT(&(info[1]),  i);
		SETSYMBOL(&(info[2]), x->link[i].Id);
		SETFLOAT(&(info[3]),  x->link[i].active);
		SETFLOAT(&(info[4]),  x->link[i].mass1->num);
		SETFLOAT(&(info[5]),  x->link[i].mass2->num);
		SETFLOAT(&(info[6]),  x->link[i].K);
		SETFLOAT(&(info[7]),  x->link[i].D);
		
		switch(x->link[i].lType)
		{
		case 0 :
			SETSYMBOL(&(info[0]),  gensym("link"));
			SETFLOAT(&(info[8]),   x->link[i].Pow);
			SETFLOAT(&(info[9]),   x->link[i].L);
			SETFLOAT(&(info[10]),   x->link[i].Lmin);
			SETFLOAT(&(info[11]),  x->link[i].Lmax);
			outlet_anything(x->main_outlet, gensym("linkInfo"), 12, info);			
			break;
		case 1 :
			SETSYMBOL(&(info[0]), gensym("tLink"));
			SETFLOAT(&(info[8]),   x->link[i].Pow);
			SETFLOAT(&(info[9]),   x->link[i].L);
			SETFLOAT(&(info[10]),   x->link[i].Lmin);
			SETFLOAT(&(info[11]),  x->link[i].Lmax);
			SETFLOAT(&(info[12]),  x->link[i].VX);
			SETFLOAT(&(info[13]),  x->link[i].VY);				
			outlet_anything(x->main_outlet, gensym("linkInfo"), 14, info);	
			break;
		case 2 :
			SETSYMBOL(&(info[0]), gensym("tabLink"));
			SETSYMBOL(&(info[8]), x->link[i].arrayK);
			SETFLOAT(&(info[9]),  x->link[i].K_L);
			SETSYMBOL(&(info[10]), x->link[i].arrayD);
			SETFLOAT(&(info[11]), x->link[i].D_L);
			outlet_anything(x->main_outlet, gensym("linkInfo"), 12, info);	
		}
	}
}

void pmpd3d_massNumber(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	t_atom info[1];
	int i, j;
	
    if (argc==0) 
    {		
		SETFLOAT(&(info[0]), x->nb_mass);
		outlet_anything(x->main_outlet, gensym("massNumber"), 1, info);
	}
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
		j=0;
		for(i=0; i < x->nb_mass; i++)
		{
            if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id) 
			{
				j++;
			}
		}
		SETFLOAT(&(info[0]), j);
		outlet_anything(x->main_outlet, gensym("massNumber"), 1, info);
	}
}

void pmpd3d_linkNumber(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	t_atom info[1];
	int i, j ;
	
    if (argc==0) 
    {
		SETFLOAT(&(info[0]), x->nb_link);
		outlet_anything(x->main_outlet, gensym("linkNumber"), 1, info);
	}
    else if ((argc==1) && (argv[0].a_type == A_SYMBOL)) 
    {
		j=0;
		for(i=0; i < x->nb_link; i++)
		{
            if (atom_getsymbolarg(0,argc,argv) == x->link[i].Id) 
			{
				j++;
			}	
		}
		SETFLOAT(&(info[0]), j);
		outlet_anything(x->main_outlet, gensym("linkNumber"), 1, info);
	}
}
