void pmpd2d_massesPosL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[2*x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[2*i]  ),x->mass[i].posX);
        SETFLOAT(&(pos_list[2*i+1]),x->mass[i].posY);
    }
    outlet_anything(x->main_outlet, gensym("massesPosL"),2*x->nb_mass , pos_list);
}

void pmpd2d_massesForcesL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[2*x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[2*i]  ),x->mass[i].forceX);
        SETFLOAT(&(pos_list[2*i+1]),x->mass[i].forceY);
    }
    outlet_anything(x->main_outlet, gensym("massesForcesL"),2*x->nb_mass , pos_list);
}

void pmpd2d_massesSpeedsL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[2*x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[2*i]  ),x->mass[i].speedX);
        SETFLOAT(&(pos_list[2*i+1]),x->mass[i].speedY);
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsL"),2*x->nb_mass , pos_list);
}

void pmpd2d_massesPosXL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].posX);
    }
    outlet_anything(x->main_outlet, gensym("massesPosXL"),x->nb_mass , pos_list);
}

void pmpd2d_massesForcesXL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].forceX);
    }
    outlet_anything(x->main_outlet, gensym("massesForcesXL"),x->nb_mass , pos_list);
}

void pmpd2d_massesSpeedsXL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].speedX);
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsXL"),x->nb_mass , pos_list);
}

void pmpd2d_massesPosYL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].posY);
    }
    outlet_anything(x->main_outlet, gensym("massesPosYL"),x->nb_mass , pos_list);
}

void pmpd2d_massesForcesYL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].forceY);
    }
    outlet_anything(x->main_outlet, gensym("massesForcesYL"),x->nb_mass , pos_list);
}

void pmpd2d_massesSpeedsYL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),x->mass[i].speedY);
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsYL"),x->nb_mass , pos_list);
}

void pmpd2d_massesPosNormL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i < x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].posX)+sqr(x->mass[i].posY)));
    }
    outlet_anything(x->main_outlet, gensym("massesPosNormL"),x->nb_mass , pos_list);
}

void pmpd2d_massesForcesNormL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].forceX)+sqr(x->mass[i].forceY)));
    }
    outlet_anything(x->main_outlet, gensym("massesForcesNormL"),x->nb_mass , pos_list);
}

void pmpd2d_massesSpeedsNormL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_mass];

    for (i=0; i< x->nb_mass; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt(sqr(x->mass[i].speedX)+sqr(x->mass[i].speedY)));
    }
    outlet_anything(x->main_outlet, gensym("massesSpeedsNormL"),x->nb_mass , pos_list);
}

void pmpd2d_massesPosMean(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, somme;
    t_int i,j;
    t_atom mean[3];

    sommeX = 0;
    sommeY = 0;
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
                somme +=  sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY)); // distance au centre
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
                somme +=  sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY)); // distance au centre
                j++;
        }
    }    
    
    sommeX /= j;
    sommeY /= j;
    somme  /= j;    
    
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),somme);
    
    outlet_anything(x->main_outlet, gensym("massesPosMean"),3 , mean);
}

void pmpd2d_massesPosStd(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, somme;
    t_int i,j;
    t_float stdX, stdY,std;
    t_atom std_out[3];

    sommeX = 0;
    sommeY = 0;
    somme  = 0;
    stdX = 0;
    stdY = 0;
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
                somme +=  sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY)); // distance au centre
                j++;
            }
        }
        sommeX /= j;
        sommeY /= j;
        somme /= j;
        for (i=0; i< x->nb_mass; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            { 
                stdX += sqr(x->mass[i].posX-sommeX);
                stdY += sqr(x->mass[i].posY-sommeY);
                std  +=  sqr(sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY))-somme);
            }
        }        
    }
    else
    {
        for (i=0; i< x->nb_mass; i++)
        {
            sommeX += x->mass[i].posX;
            sommeY += x->mass[i].posY;
            somme +=  sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY)); // distance au centre
            j++;
        }
        sommeX /= j;
        sommeY /= j;
        somme /= j;
        for (i=0; i< x->nb_mass; i++)
        {
            stdX += sqr(x->mass[i].posX-sommeX);
            stdY += sqr(x->mass[i].posY-sommeY);
            std  += sqr(sqrt(sqr(x->mass[i].posX) + sqr(x->mass[i].posY))-somme);
        }
    }    
    
    stdX = sqrt(stdX/j);
    stdY = sqrt(stdY/j);
    std  = sqrt(std /j);    

    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),std);
    
    outlet_anything(x->main_outlet, gensym("massesPosStd"),3 , std_out);
}

void pmpd2d_massesForcesMean(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, somme;
    t_int i,j;
    t_atom mean[3];

    sommeX = 0;
    sommeY = 0;
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
                somme +=  sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY)); // distance au centre
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
                somme +=  sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY)); // distance au centre
                j++;
        }
    }    
    
    sommeX /= j;
    sommeY /= j;
    somme  /= j;    
    
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),somme);
    
    outlet_anything(x->main_outlet, gensym("massesForcesMean"),3 , mean);
}

void pmpd2d_massesForcesStd(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, somme;
    t_int i,j;
    t_float stdX, stdY,std;
    t_atom std_out[3];

    sommeX = 0;
    sommeY = 0;
    somme  = 0;
    stdX = 0;
    stdY = 0;
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
                somme +=  sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY)); // distance au centre
                j++;
            }
        }
        sommeX /= j;
        sommeY /= j;
        somme /= j;
        for (i=0; i< x->nb_mass; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            { 
                stdX += sqr(x->mass[i].forceX-sommeX);
                stdY += sqr(x->mass[i].forceY-sommeY);
                std  +=  sqr(sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY))-somme);
            }
        }        
    }
    else
    {
        for (i=0; i< x->nb_mass; i++)
        {
            sommeX += x->mass[i].forceX;
            sommeY += x->mass[i].forceY;
            somme +=  sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY)); // distance au centre
            j++;
        }
        sommeX /= j;
        sommeY /= j;
        somme /= j;
        for (i=0; i< x->nb_mass; i++)
        {
            stdX += sqr(x->mass[i].forceX-sommeX);
            stdY += sqr(x->mass[i].forceY-sommeY);
            std  += sqr(sqrt(sqr(x->mass[i].forceX) + sqr(x->mass[i].forceY))-somme);
        }
    }    
    
    stdX = sqrt(stdX/j);
    stdY = sqrt(stdY/j);
    std  = sqrt(std /j);    

    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),std);
    
    outlet_anything(x->main_outlet, gensym("massesForcesStd"),3 , std_out);
}

void pmpd2d_massesSpeedsMean(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, somme;
    t_int i,j;
    t_atom mean[3];

    sommeX = 0;
    sommeY = 0;
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
                somme +=  sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY)); // distance au centre
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
                somme +=  sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY)); // distance au centre
                j++;
        }
    }    
    
    sommeX /= j;
    sommeY /= j;
    somme  /= j;    
    
    SETFLOAT(&(mean[0]),sommeX);
    SETFLOAT(&(mean[1]),sommeY);
    SETFLOAT(&(mean[2]),somme);
    
    outlet_anything(x->main_outlet, gensym("massesSpeedsMean"),3 , mean);
}

void pmpd2d_massesSpeedsStd(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
    t_float sommeX, sommeY, somme;
    t_int i,j;
    t_float stdX, stdY,std;
    t_atom std_out[3];

    sommeX = 0;
    sommeY = 0;
    somme  = 0;
    stdX = 0;
    stdY = 0;
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
                somme +=  sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY)); // distance au centre
                j++;
            }
        }
        sommeX /= j;
        sommeY /= j;
        somme /= j;
        for (i=0; i< x->nb_mass; i++)
        {
            if (atom_getsymbolarg(0,argc,argv) == x->mass[i].Id)
            { 
                stdX += sqr(x->mass[i].speedX-sommeX);
                stdY += sqr(x->mass[i].speedY-sommeY);
                std  +=  sqr(sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY))-somme);
            }
        }        
    }
    else
    {
        for (i=0; i< x->nb_mass; i++)
        {
            sommeX += x->mass[i].speedX;
            sommeY += x->mass[i].speedY;
            somme +=  sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY)); // distance au centre
            j++;
        }
        sommeX /= j;
        sommeY /= j;
        somme /= j;
        for (i=0; i< x->nb_mass; i++)
        {
            stdX += sqr(x->mass[i].speedX-sommeX);
            stdY += sqr(x->mass[i].speedY-sommeY);
            std  += sqr(sqrt(sqr(x->mass[i].speedX) + sqr(x->mass[i].speedY))-somme);
        }
    }    
    
    stdX = sqrt(stdX/j);
    stdY = sqrt(stdY/j);
    std  = sqrt(std /j);    

    SETFLOAT(&(std_out[0]),stdX);
    SETFLOAT(&(std_out[1]),stdY);
    SETFLOAT(&(std_out[2]),std);
    
    outlet_anything(x->main_outlet, gensym("massesSpeedsStd"),3 , std_out);
}

// --------------------------------------------

void pmpd2d_linksPosL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[2*x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[2*i]  ),(x->link[i].mass2->posX + x->link[i].mass1->posX)/2);
        SETFLOAT(&(pos_list[2*i+1]),(x->link[i].mass2->posY + x->link[i].mass1->posY)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosL"),2*x->nb_link , pos_list);
}

void pmpd2d_linksLengthL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[2*x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[2*i]  ),x->link[i].mass2->posX - x->link[i].mass1->posX);
        SETFLOAT(&(pos_list[2*i+1]),x->link[i].mass2->posY - x->link[i].mass1->posY);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthL"),2*x->nb_link , pos_list);
}

void pmpd2d_linksPosSpeedL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[2*x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[2*i]  ),(x->link[i].mass2->speedX + x->link[i].mass1->speedX)/2);
        SETFLOAT(&(pos_list[2*i+1]),(x->link[i].mass2->speedY + x->link[i].mass1->speedY)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedL"),3*x->nb_link , pos_list);
}

void pmpd2d_linksLengthSpeedL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[2*x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[2*i]  ),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
        SETFLOAT(&(pos_list[2*i+1]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedL"),2*x->nb_link , pos_list);
}

void pmpd2d_linksPosXL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->posX + x->link[i].mass2->posX)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosXL"),x->nb_link , pos_list);
}

void pmpd2d_linksLengthXL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->posX - x->link[i].mass1->posX);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthXL"),x->nb_link , pos_list);
}

void pmpd2d_linksPosSpeedXL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedXL"),x->nb_link , pos_list);
}

void pmpd2d_linksLengthSpeedXL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->speedX - x->link[i].mass1->speedX);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedXL"),x->nb_link , pos_list);
}

void pmpd2d_linksPosYL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->posY + x->link[i].mass2->posY)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosYL"),x->nb_link , pos_list);
}

void pmpd2d_linksLengthYL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->posY - x->link[i].mass1->posY);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthYL"),x->nb_link , pos_list);
}

void pmpd2d_linksPosSpeedYL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),(x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2);
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedYL"),x->nb_link , pos_list);
}

void pmpd2d_linksLengthSpeedYL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),x->link[i].mass2->speedY - x->link[i].mass1->speedY);
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedYL"),x->nb_link , pos_list);
}

void pmpd2d_linksPosNormL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr((x->link[i].mass1->posX + x->link[i].mass2->posX)/2) + \
                            sqr((x->link[i].mass1->posY + x->link[i].mass2->posY)/2) ));
    }
    outlet_anything(x->main_outlet, gensym("linksPosNormL"),x->nb_link , pos_list);
}

void pmpd2d_linksLengthNormL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr(x->link[i].mass2->posX - x->link[i].mass1->posX) + \
                            sqr(x->link[i].mass2->posY - x->link[i].mass1->posY)  ));
    }
    outlet_anything(x->main_outlet, gensym("linksLengthNormL"),x->nb_link , pos_list);
}

void pmpd2d_linksPosSpeedNormL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr((x->link[i].mass1->speedX + x->link[i].mass2->speedX)/2) + \
                            sqr((x->link[i].mass1->speedY + x->link[i].mass2->speedY)/2)  ));
    }
    outlet_anything(x->main_outlet, gensym("linksPosSpeedNormL"),x->nb_link , pos_list);
}

void pmpd2d_linksLengthSpeedNormL(t_pmpd2d *x)
{
    int i;
    t_atom pos_list[x->nb_link];

    for (i=0; i < x->nb_link; i++)
    {
        SETFLOAT(&(pos_list[i]),sqrt( \
                            sqr(x->link[i].mass2->speedX - x->link[i].mass1->speedX) + \
                            sqr(x->link[i].mass2->speedY - x->link[i].mass1->speedY) ));
    }
    outlet_anything(x->main_outlet, gensym("linksLengthSpeedNormL"),x->nb_link , pos_list);
}

