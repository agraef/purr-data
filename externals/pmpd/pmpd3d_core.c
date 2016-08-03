/*
 *  pmpd3d_core.c
 */


t_float sign_ch(t_float v)
{
    return v > 0 ? 1 : -1;
}

t_float sqr(t_float x)
{
    return x*x ;
}

t_float pow_ch(t_float x, t_float y)
{
    return x > 0 ? pow(x,y) : -pow(-x,y);
}

t_float mix(t_float X, t_float Y, t_float mix)
{
    return (1-mix)*X + mix*Y ;
}

t_float tabread2(t_pmpd3d *x, t_float pos, t_symbol *array)
{
    t_garray *a;
    int npoints;
    t_word *vec;
    t_float posx;
    
    if (!(a = (t_garray *)pd_findbyclass(array, garray_class)))
        pd_error(x, "%s: no such array", array->s_name);
    else if (!garray_getfloatwords(a, &npoints, &vec))
        pd_error(x, "%s: bad template for tabLink", array->s_name);
    else
    {
        posx = fabs(pos)*npoints;
        int n=posx;
        if (n >= npoints - 1) 
            return (sign_ch(pos)*vec[npoints-1].w_float);
        float fract = posx-n;
        return (sign_ch(pos) * ( fract*vec[n+1].w_float+(1-fract)*vec[n].w_float));
    }
    return( pos); // si il y a un pb sur le tableau, on renvoie l'identité
}

void pmpd3d_reset(t_pmpd3d *x)
{
    x->nb_link = 0;
    x->nb_mass = 0;
    x->minX = -1000000;
    x->maxX = 1000000;
    x->minY = -1000000;
    x->maxY = 1000000;
    x->minZ = -1000000;
    x->maxZ = 1000000;
    x->grab = 0;
}

void *pmpd3d_new(t_symbol *s, int argc, t_atom *argv)
{
	t_float tmp;
    t_pmpd3d *x = (t_pmpd3d *)pd_new(pmpd3d_class);
	
    pmpd3d_reset(x);
    
    x->main_outlet=outlet_new(&x->x_obj, 0);
    // x->info_outlet=outlet_new(&x->x_obj, 0); // TODO
	
	x->nb_max_mass = 10000;
	x->nb_max_link = 10000;
	
	if((argc >= 1) && (argv[0].a_type == A_FLOAT)){
		tmp = atom_getfloatarg(0, argc, argv);
		if (tmp >= 0) {
			x->nb_max_mass = tmp;
			x->nb_max_link = tmp;
		}
	}
	if((argc >= 2) && (argv[1].a_type == A_FLOAT)){
		tmp = atom_getfloatarg(1, argc, argv);
		if (tmp >= 0) {
			x->nb_max_link = tmp;
		}
	}
		
 	x->mass = getbytes(sizeof(massStruct)*x->nb_max_mass);
 	x->link = getbytes(sizeof(linkStruct)*x->nb_max_link);

    return (void *)x;
}

void pmpd3d_bang(t_pmpd3d *x)
{
	// this part is doing all the PM
    t_float F, L, Lx,Ly, Lz, Fx, Fy, Fz, tmp, tmpX, tmpY, tmpZ, speed;
    t_int i;
    // post("bang");
	
    for (i=0; i<x->nb_mass; i++)
		// compute new masses position
        if (x->mass[i].mobile > 0) // only if mobile
        {
			// amplify force that opose to movement
			if (x->mass[i].overdamp != 0)
			{
				tmp = x->mass[i].speedX * x->mass[i].forceX + x->mass[i].speedY * x->mass[i].forceY + x->mass[i].speedZ * x->mass[i].forceZ;
				tmp = min(0,tmp); // overdamped only if force opose movment
				tmp *= -x->mass[i].overdamp;
				tmp += 1;
				x->mass[i].forceX *= tmp;
				x->mass[i].forceY *= tmp;
				x->mass[i].forceZ *= tmp;
			}
			
			// compute new velocity thanks to forces. (Forces = M * acceleration) 
            x->mass[i].speedX += x->mass[i].forceX * x->mass[i].invM;
            x->mass[i].speedY += x->mass[i].forceY * x->mass[i].invM;
            x->mass[i].speedZ += x->mass[i].forceZ * x->mass[i].invM;
            
            // no need to reset force to 0, because we compute a new force latter thanks to velocity damping
            // x->mass[i].forceX = 0;
            // x->mass[i].forceY = 0;        
            // x->mass[i].forceZ = 0;        
            
            // compute new speed thanks to new velocity
            x->mass[i].posX += x->mass[i].speedX ;
            x->mass[i].posY += x->mass[i].speedY ;
            x->mass[i].posZ += x->mass[i].speedZ ;
            
            // space limitation
            if ( (x->mass[i].posX < x->minX) || (x->mass[i].posX > x->maxX) || (x->mass[i].posY < x->minY) 
                || (x->mass[i].posY > x->maxY) || (x->mass[i].posZ < x->minZ) || (x->mass[i].posZ > x->maxZ) ) 
            {
                tmpX = min(x->maxX,max(x->minX,x->mass[i].posX));
                tmpY = min(x->maxY,max(x->minY,x->mass[i].posY));
                tmpZ = min(x->maxZ,max(x->minZ,x->mass[i].posZ));
                x->mass[i].speedX -= x->mass[i].posX - tmpX;
                x->mass[i].speedY -= x->mass[i].posY - tmpY;
                x->mass[i].speedZ -= x->mass[i].posZ - tmpZ;
                x->mass[i].posX = tmpX;
                x->mass[i].posY = tmpY;
                x->mass[i].posZ = tmpZ;
            }
            
            // velocity damping of every masse (set a new force)
            x->mass[i].forceX = -x->mass[i].D2 * x->mass[i].speedX;
            x->mass[i].forceY = -x->mass[i].D2 * x->mass[i].speedY;
            x->mass[i].forceZ = -x->mass[i].D2 * x->mass[i].speedZ;
            
            // offset on velocity damping (to impose a specific velocity)
            speed = sqrt(x->mass[i].speedX * x->mass[i].speedX + x->mass[i].speedY * x->mass[i].speedY + x->mass[i].speedZ * x->mass[i].speedZ);
            if (speed != 0) {
                x->mass[i].forceX += x->mass[i].D2offset * (x->mass[i].speedX/speed);
                x->mass[i].forceY += x->mass[i].D2offset * (x->mass[i].speedY/speed);
                x->mass[i].forceZ += x->mass[i].D2offset * (x->mass[i].speedZ/speed);
            }
        }
	
    for (i=0; i<x->nb_link; i++)
    { // compute link forces
		if (x->link[i].active == 1)
        {
			Lx = x->link[i].mass1->posX - x->link[i].mass2->posX;
			Ly = x->link[i].mass1->posY - x->link[i].mass2->posY;
			Lz = x->link[i].mass1->posZ - x->link[i].mass2->posZ;
			L = sqrt( sqr(Lx) + sqr(Ly) + sqr(Lz) );
			
			if ( (L >= x->link[i].Lmin) && (L < x->link[i].Lmax)  && (L != 0))
			{
				if (x->link[i].lType == 2)
				{ // K et D viennent d'une table
					F  = x->link[i].D * tabread2(x, (L - x->link[i].distance) / x->link[i].D_L, x->link[i].arrayD);
					F += x->link[i].K * tabread2(x, L / x->link[i].K_L, x->link[i].arrayK);
				}
				else
				{            
					F  = x->link[i].D * (L - x->link[i].distance) ;
					F += x->link[i].K *  pow_ch( L - x->link[i].L, x->link[i].Pow);
				}
				
				Fx = F * Lx/L;
				Fy = F * Ly/L;    
				Fz = F * Lz/L;    
                
				if (x->link[i].lType == 1)
				{ // on projette selon 1 axe
					Fx = Fx*x->link[i].VX; // V est unitaire, dc on projete sans pb
					Fy = Fy*x->link[i].VY;                
					Fz = Fz*x->link[i].VZ;                
				}
				
				x->link[i].mass1->forceX -= Fx;
				x->link[i].mass1->forceY -= Fy;
				x->link[i].mass1->forceZ -= Fz;
				x->link[i].mass2->forceX += Fx;
				x->link[i].mass2->forceY += Fy;
				x->link[i].mass2->forceZ += Fz;
				x->link[i].forceX = Fx; // save for latter use
				x->link[i].forceY = Fy;
				x->link[i].forceZ = Fz;
			}
			x->link[i].distance=L;
		}
	}
}

void pmpd3d_mass(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
// t_symbol *Id, t_float mobile, t_float M, t_float posX, t_float posY, t_float posZ )
{ 
 	if ( x->nb_mass >= x->nb_max_mass) {
 		x->nb_mass = x->nb_max_mass-1; 
 		pd_error(x, "pmpd3d masses number exceeded, please increase max masses number");
 	}
	x->mass[x->nb_mass].Id = gensym("mass");
	if ((argc >= 1) && (argv[0].a_type == A_SYMBOL))
		x->mass[x->nb_mass].Id = atom_getsymbolarg(0,argc,argv);
	x->mass[x->nb_mass].mobile = 1;
	if ((argc >= 2) &&  (argv[1].a_type == A_FLOAT))
		x->mass[x->nb_mass].mobile = (int) atom_getfloatarg(1, argc, argv);
	t_float M = 1;
	if ((argc >= 3) && (argv[2].a_type == A_FLOAT))
		M = atom_getfloatarg(2, argc, argv);
	if (M<=0) M=1;
	x->mass[x->nb_mass].invM = 1/M;
	x->mass[x->nb_mass].speedX = 0;
	x->mass[x->nb_mass].speedY = 0;
	x->mass[x->nb_mass].speedZ = 0;
	x->mass[x->nb_mass].posX = 0;
	if ((argc >= 4) &&  (argv[3].a_type == A_FLOAT))
		x->mass[x->nb_mass].posX = atom_getfloatarg(3, argc, argv);
	x->mass[x->nb_mass].posY = 0;
	if ((argc >= 5) &&  (argv[4].a_type == A_FLOAT))
		x->mass[x->nb_mass].posY = atom_getfloatarg(4, argc, argv);
	x->mass[x->nb_mass].posZ = 0;
	if ((argc >= 6) &&  (argv[5].a_type == A_FLOAT))
		x->mass[x->nb_mass].posZ = atom_getfloatarg(5, argc, argv);
	x->mass[x->nb_mass].forceX = 0;
	x->mass[x->nb_mass].forceY = 0;
	x->mass[x->nb_mass].forceZ = 0;
	x->mass[x->nb_mass].num = x->nb_mass;
	x->mass[x->nb_mass].D2 = 0;
	x->mass[x->nb_mass].D2offset = 0;
	x->mass[x->nb_mass].overdamp = 0;
	x->nb_mass++ ;
}

void pmpd3d_create_link(t_pmpd3d *x, t_symbol *Id, int mass1, int mass2, t_float K, t_float D, t_float Pow, t_float Lmin, t_float Lmax, t_int type)
{ // create a link based on mass number
	
    if ((x->nb_mass>1) && (mass1 != mass2) && (mass1 >= 0) && (mass2 >= 0) && (mass1 < x->nb_mass) && (mass2 < x->nb_mass) )
    {
		if ( x->nb_link >= x->nb_max_link) {
 			x->nb_link = x->nb_max_link-1; 
 			pd_error(x, "pmpd3d links number exceeded, please increase max links number");
 		}
 		
        x->link[x->nb_link].lType = type;
        x->link[x->nb_link].Id = Id;
        x->link[x->nb_link].active = 1;
        x->link[x->nb_link].mass1 = &x->mass[mass1]; 
        x->link[x->nb_link].mass2 = &x->mass[mass2];
        x->link[x->nb_link].K = K;
        x->link[x->nb_link].D = D;
        x->link[x->nb_link].L = sqrt(sqr(x->mass[mass1].posX - x->mass[mass2].posX) + 
									 sqr(x->mass[mass1].posY - x->mass[mass2].posY) + sqr(x->mass[mass1].posZ - x->mass[mass2].posZ));
        x->link[x->nb_link].Pow = Pow;
        x->link[x->nb_link].Lmin = Lmin;
        x->link[x->nb_link].Lmax = Lmax;
        x->link[x->nb_link].distance = x->link[x->nb_link].L ;
        x->link[x->nb_link].forceX = 0 ;
        x->link[x->nb_link].forceY = 0 ;
        x->link[x->nb_link].forceZ = 0 ;
        x->nb_link++ ;
    }
}

void pmpd3d_link(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{ // add a link : Id, *mass1, *mass2, K, D, Pow, Lmin, Lmax;
	
    int i, j;
	
	t_symbol *Id = gensym("link");
	if ((argc >= 1) &&  (argv[0].a_type == A_SYMBOL))
		Id = atom_getsymbolarg(0,argc,argv);
	t_float K = 0;
   	if ((argc >= 4) &&  (argv[3].a_type == A_FLOAT))
		K = atom_getfloatarg(3, argc, argv);
	t_float D = 0;
   	if ((argc >= 5) &&  (argv[4].a_type == A_FLOAT))
		D = atom_getfloatarg(4, argc, argv);
    t_float Pow = 1; 
    if ((argc > 5) &&  (argv[5].a_type == A_FLOAT)) 
		Pow = atom_getfloatarg(5, argc, argv);
    t_float Lmin = -1000000;
    if ((argc > 6) &&  (argv[6].a_type == A_FLOAT)) 
		Lmin = atom_getfloatarg(6, argc, argv);
    t_float Lmax =  1000000;
    if ((argc > 7)  &&  (argv[7].a_type == A_FLOAT)) 
		Lmax = atom_getfloatarg(7, argc, argv);
	//    post("%d,%d, %f,%f", mass1, mass2, K, D);
	
    if ( (argc >= 3) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) )
    {
        pmpd3d_create_link(x, Id, atom_getfloatarg(1, argc, argv), atom_getfloatarg(2, argc, argv), K, D, Pow, Lmin, Lmax, 0);
    }
    else if ( (argc >= 3) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
			{
				pmpd3d_create_link(x, Id, i, atom_getfloatarg(2, argc, argv), K, D, Pow, Lmin, Lmax, 0);
			}
		}
	}
	else if ( (argc >= 3) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_SYMBOL ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(2,argc,argv) == x->mass[i].Id)
			{
				pmpd3d_create_link(x, Id, atom_getfloatarg(1, argc, argv), i, K, D, Pow, Lmin, Lmax, 0);
			}
		}
	}
	else if ( (argc >= 3) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_SYMBOL ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			for (j=0; j< x->nb_mass; j++)
			{
				if ( (atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)&(atom_getsymbolarg(2,argc,argv) == x->mass[j].Id))
				{
					if (!( (x->mass[i].Id == x->mass[j].Id) && (i>j) )) 
						// si lien entre 2 serie de masses identique entres elle, alors on ne creer qu'un lien sur 2, pour evider les redondances
						pmpd3d_create_link(x, Id, i, j, K, D, Pow, Lmin, Lmax, 0);
				}
			}   
		}
	}
	else 
		pmpd3d_create_link(x, Id, 0, 1, K, D, Pow, Lmin, Lmax, 0);
}

void pmpd3d_tLink(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{ // add a link : Id, *mass1, *mass2, K, D, Vx, Vy, Pow, Lmin, Lmax;
	
    int i, j;
    
    t_symbol *Id = atom_getsymbolarg(0,argc,argv);
    int mass1 = atom_getfloatarg(1, argc, argv);
    int mass2 = atom_getfloatarg(2, argc, argv);
    t_float K = atom_getfloatarg(3, argc, argv);
    t_float D = atom_getfloatarg(4, argc, argv);
    t_float vecteurX = atom_getfloatarg(5, argc, argv);
    t_float vecteurY = atom_getfloatarg(6, argc, argv);
    t_float vecteurZ = atom_getfloatarg(7, argc, argv);
    t_float vecteur = sqrt( sqr(vecteurX) + sqr(vecteurY) + sqr(vecteurZ) );
    vecteurX /= vecteur;
    vecteurY /= vecteur;
    vecteurZ /= vecteur;
    t_float Pow = 1; 
    if (argc > 8) Pow = atom_getfloatarg(8, argc, argv);
    t_float Lmin = 0;
    if (argc > 9) Lmin = atom_getfloatarg(9, argc, argv);
    t_float Lmax =  1000000;
    if (argc > 10) Lmax = atom_getfloatarg(10, argc, argv);
	
    if ( (argc >= 3) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) )
    {
        pmpd3d_create_link(x, Id, mass1, mass2, K, D, Pow, Lmin, Lmax, 1);
        x->link[x->nb_link-1].VX = vecteurX;
        x->link[x->nb_link-1].VY = vecteurY;
        x->link[x->nb_link-1].VZ = vecteurZ;
    }
    else if ( (argc >= 3) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
			{
				pmpd3d_create_link(x, Id, i, mass2, K, D, Pow, Lmin, Lmax, 1);
				x->link[x->nb_link-1].VX = vecteurX;
				x->link[x->nb_link-1].VY = vecteurY;
				x->link[x->nb_link-1].VZ = vecteurZ;
			}
		}
	}
	else if ( (argc >= 3) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_SYMBOL ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(2,argc,argv) == x->mass[i].Id)
			{
				pmpd3d_create_link(x, Id, mass1, i, K, D, Pow, Lmin, Lmax, 1);
				x->link[x->nb_link-1].VX = vecteurX;
				x->link[x->nb_link-1].VY = vecteurY;
				x->link[x->nb_link-1].VZ = vecteurZ;
			}
		}
	}
	else if ( (argc >= 3) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_SYMBOL ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			for (j=0; j< x->nb_mass; j++)
			{
				if ( (atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)&(atom_getsymbolarg(2,argc,argv) == x->mass[j].Id))
				{
					if (!( (x->mass[i].Id == x->mass[j].Id) && (i>j) )) 
						// si lien entre 2 serie de masses identique entres elle, alors on ne creer qu'un lien sur 2, pour evider les redondances
					{
						pmpd3d_create_link(x, Id, i, j, K, D, Pow, Lmin, Lmax, 1);
						x->link[x->nb_link-1].VX = vecteurX;
						x->link[x->nb_link-1].VY = vecteurY;
						x->link[x->nb_link-1].VZ = vecteurZ;
					}
				}
			}   
		}
	}
}

void pmpd3d_tabLink(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
    int i, j;
    
    t_symbol *Id = atom_getsymbolarg(0,argc,argv);
    int mass1 = atom_getfloatarg(1, argc, argv);
    int mass2 = atom_getfloatarg(2, argc, argv);
    t_symbol *arrayK = atom_getsymbolarg(3,argc,argv);
    t_float Kl = atom_getfloatarg(4, argc, argv);
    if (Kl <= 0) Kl = 1;
    t_symbol *arrayD = atom_getsymbolarg(5,argc,argv);    
    t_float Dl = atom_getfloatarg(6, argc, argv);
    if (Dl <= 0) Dl = 1;
	
    if ( ( argc > 2 ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) )
    {
        pmpd3d_create_link(x, Id, mass1, mass2, 1, 1, 1, 0, 1000000, 2);
        x->link[x->nb_link-1].arrayK = arrayK;
        x->link[x->nb_link-1].arrayD = arrayD;
        x->link[x->nb_link-1].K_L = Kl;
        x->link[x->nb_link-1].D_L = Dl;        
    }
    else if ( ( argc > 2 ) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_FLOAT ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
			{
				pmpd3d_create_link(x, Id, i, mass2, 1, 1, 1, 0, 1000000, 2);
				x->link[x->nb_link-1].arrayK = arrayK;
				x->link[x->nb_link-1].arrayD = arrayD;
				x->link[x->nb_link-1].K_L = Kl;
				x->link[x->nb_link-1].D_L = Dl;    
			}
		}
	}
	else if ( ( argc > 2 ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_SYMBOL ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			if ( atom_getsymbolarg(2,argc,argv) == x->mass[i].Id)
			{
				pmpd3d_create_link(x, Id, mass1, i, 1, 1, 1, 0, 1000000, 2);
				x->link[x->nb_link-1].arrayK = arrayK;
				x->link[x->nb_link-1].arrayD = arrayD;
				x->link[x->nb_link-1].K_L = Kl;
				x->link[x->nb_link-1].D_L = Dl;    
			}
		}
	}
	else if ( ( argc > 2 ) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_SYMBOL ) )
	{
		for (i=0; i< x->nb_mass; i++)
		{
			for (j=0; j< x->nb_mass; j++)
			{
				if ( (atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)&(atom_getsymbolarg(2,argc,argv) == x->mass[j].Id))
				{
					if (!( (x->mass[i].Id == x->mass[j].Id) && (i>j) )) 
						// si lien entre 2 serie de masses identique entres elle, alors on ne creer qu'un lien sur 2, pour evider les redondances
					{
						pmpd3d_create_link(x, Id, i, j, 1, 1, 1, 0, 1000000, 2);
						x->link[x->nb_link-1].arrayK = arrayK;
						x->link[x->nb_link-1].arrayD = arrayD;
						x->link[x->nb_link-1].K_L = Kl;
						x->link[x->nb_link-1].D_L = Dl;    
					}
				}
			}   
		}
	}
}

void pmpd3d_delLink_int(t_pmpd3d *x, int dellink)
{
	int i;
	if ( ( dellink < x->nb_link ) && ( dellink >= 0) )
	{
		x->nb_link--;
		for (i=dellink; i < x->nb_link; i++)
		x->link[i]=x->link[i+1]; 
	}
}

void pmpd3d_delLink(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	int i,nb_toremove;
	if ( (argc > 0) && ( argv[0].a_type == A_FLOAT ) )
		pmpd3d_delLink_int(x, atom_getfloatarg(0, argc, argv));
	if ( (argc > 0) && ( argv[0].a_type == A_SYMBOL ) )
/*		for (i=0; i<x->nb_link; )
			if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id )
				pmpd3d_delLink_int(x, i);
			else i++;
*/
	{
		nb_toremove=0;
		for (i=0; i < x->nb_link; i++)
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id )
			{
				nb_toremove++;
			}
			else			
			{	
				if (nb_toremove > 0)
				{
					x->link[i-nb_toremove]=x->link[i]; 
				}
			}
		}
		x->nb_link -= nb_toremove;
	}
}

void pmpd3d_delMass_int(t_pmpd3d *x, int delmass)
{
	int i,nb_toremove;

	if ( ( delmass < x->nb_mass ) && ( delmass >= 0) )
	{
	/*	for (i=0; i < x->nb_link; ) // delete link connected to the mass to delete
		{
			if ( (x->link[i].mass1->num == delmass) || (x->link[i].mass2->num == delmass) )
			pmpd3d_delLink_int(x, i);
			else i++;
			// post("loop %d sur %d", i, x->nb_link);
		}*/
		nb_toremove=0;
		for (i=0; i < x->nb_link; i++)
		{
			if ( (x->link[i].mass1->num == delmass) || (x->link[i].mass2->num == delmass) )
			{
				nb_toremove++;
			}
			else			
			{	
				if (nb_toremove > 0)
				{
					x->link[i-nb_toremove]=x->link[i]; 
				}
			}
		}
		x->nb_link -= nb_toremove;
		
		for (i=0; i < x->nb_link; i++) // change pointer to mass that index moved
		{
			if (x->link[i].mass1->num > delmass )
			{ x->link[i].mass1 = &x->mass[x->link[i].mass1->num-1]; }
			else if (x->link[i].mass2->num > delmass )
			{ x->link[i].mass2 = &x->mass[x->link[i].mass2->num-1]; }
		}
		x->nb_mass--;
		for (i=delmass; i < x->nb_mass; i++)
		{
			x->mass[i]=x->mass[i+1];
			x->mass[i].num=i;
		}
	}
}

void pmpd3d_delMass(t_pmpd3d *x, t_symbol *s, int argc, t_atom *argv)
{
	int i, j, delmass, nb_toremove;
	if ( (argc > 0) && ( argv[0].a_type == A_FLOAT ) )
		pmpd3d_delMass_int(x, atom_getfloatarg(0, argc, argv));
	if ( (argc > 0) && ( argv[0].a_type == A_SYMBOL ) )
	/*
		for (i=0; i<x->nb_mass; )
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id )
				pmpd3d_delMass_int(x, i);
			else i++;
	*/
	{
		nb_toremove=0;
		for (i=0; i < x->nb_link; i++) // revove link associated with this mass Id
		{
			if ( (x->link[i].mass1->Id == atom_getsymbolarg(0, argc, argv)) || (x->link[i].mass2->Id == atom_getsymbolarg(0, argc, argv)) )
			{
				nb_toremove++;
			}
			else			
			{	
				if (nb_toremove > 0)
				{
					x->link[i-nb_toremove]=x->link[i]; 
				}
			}
		}
		x->nb_link -= nb_toremove;
		
		nb_toremove=0;
		for (i=0; i < x->nb_mass; i++) // remove mass
		{
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id )
			{
				nb_toremove++;
				// post("remove mass %d", i);
			}
			else			
			{	
				if (nb_toremove > 0)
				{
					x->mass[i-nb_toremove]=x->mass[i]; 
					x->mass[i-nb_toremove].num = i-nb_toremove;
					for (j=0; j < x->nb_link; j++) // for every link replace mass with the new pointer
					{
						if (x->link[j].mass1->num == i )
						{ 
							// post("mass %d : relocate link %d to mass %d",i, j, i-nb_toremove);
							x->link[j].mass1 = &x->mass[i-nb_toremove]; 
						}
						if (x->link[j].mass2->num == i )
						{ 
							// post("mass %d : relocate link2 %d to mass %d",i, j, i-nb_toremove);
							x->link[j].mass2 = &x->mass[i-nb_toremove];				
						}
					}
				}
			}
		}
		x->nb_mass -= nb_toremove;
	}
}
