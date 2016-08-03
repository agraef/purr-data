/*
 *  pmpd2d_core.c
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

t_float tabread2(t_pmpd2d *x, t_float pos, t_symbol *array)
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

t_float pow2(t_float x)
{
    return(x*x);
}

t_float getAngle_bug(t_pmpd2d *x, t_int mass1, t_int mass3, t_int mass2)
{
    t_float X12, X13, X23, Y12, Y13, Y23, D;
    X12 = x->mass[mass1].posX - x->mass[mass2].posX;
    Y12 = x->mass[mass1].posY - x->mass[mass2].posY;
    X13 = x->mass[mass1].posX - x->mass[mass3].posX;
    Y13 = x->mass[mass1].posY - x->mass[mass3].posY;
    X23 = x->mass[mass2].posX - x->mass[mass3].posX;
    Y23 = x->mass[mass2].posY - x->mass[mass3].posY;
    
    D = sqrt(X13*X13 + Y13*Y13) + sqrt(X23*X23 + Y23*Y23);
    if (D =! 0)
        return(acos((X13*X23 + Y13*Y23)/D));
    else
        return(0);
}

t_float getAngle(t_pmpd2d *x, t_int mass1, t_int mass2, t_int mass3)
{
    t_float A1, A2;
    A1 = atan2( x->mass[mass1].posX - x->mass[mass2].posX, x->mass[mass1].posY - x->mass[mass2].posY);
    A2 = atan2( x->mass[mass3].posX - x->mass[mass2].posX, x->mass[mass3].posY - x->mass[mass2].posY);
    return(A2-A1);
}

t_float distance(t_pmpd2d *x, t_int mass1, t_int mass2)
{
    t_float X,Y;
    X = x->mass[mass1].posX - x->mass[mass2].posX;
    Y = x->mass[mass1].posY - x->mass[mass2].posY;
    return(sqrt(X*X+Y*Y));
}

t_float mod2Pi(t_float angle)
{ // return an angle between -pi and pi
    t_float tmp;
    tmp = fmodf(angle-3.1415926, 6.2831852);
    return(tmp > 0 ? tmp-3.1415926 : tmp+3.1415926); // TODO : mettre un vrai PI
}

void pmpd2d_reset(t_pmpd2d *x)
{
    x->nb_link = 0;
    x->nb_mass = 0;
    x->minX = -1000000;
    x->maxX = 1000000;
    x->minY = -1000000;
    x->maxY = 1000000;
    x->grab = 0;
}

void *pmpd2d_new(t_symbol *s, int argc, t_atom *argv)
{
	t_float tmp;
    t_pmpd2d *x = (t_pmpd2d *)pd_new(pmpd2d_class);

    pmpd2d_reset(x);
    
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

void pmpd2d_bang(t_pmpd2d *x)
{ // this part is doing all the PM
    t_float F, L, Dist, Lx,Ly, Fx, Fy, tmp, tmpX, tmpY,speed;
    t_int i;

    for (i=0; i<x->nb_mass; i++) // compute new masses position
    {
        if (x->mass[i].mobile > 0) // only if mobile
        {
			// amplify force that opose to movement
			if (x->mass[i].overdamp != 0)
			{
				tmp = x->mass[i].speedX * x->mass[i].forceX + x->mass[i].speedY * x->mass[i].forceY;
				tmp = min(0,tmp); // overdamped only if force opose movment
				tmp *= -x->mass[i].overdamp;
				tmp += 1;
				x->mass[i].forceX *= tmp;
				x->mass[i].forceY *= tmp;
			}

			// compute new velocity thanks to forces. (Forces = M * acceleration) 
            x->mass[i].speedX += x->mass[i].forceX * x->mass[i].invM;
            x->mass[i].speedY += x->mass[i].forceY * x->mass[i].invM;

            // no need to reset force to 0, because we compute a new force latter thanks to velocity damping
            // x->mass[i].forceX = 0;
            // x->mass[i].forceY = 0;  
    
            // compute new speed thanks to new velocity
            x->mass[i].posX += x->mass[i].speedX ;
            x->mass[i].posY += x->mass[i].speedY ;

            // space limitation
            if ( (x->mass[i].posX < x->minX) || (x->mass[i].posX > x->maxX) || (x->mass[i].posY < x->minY) || (x->mass[i].posY > x->maxY) ) 
            {
                tmpX = min(x->maxX,max(x->minX,x->mass[i].posX));
                tmpY = min(x->maxY,max(x->minY,x->mass[i].posY));
                x->mass[i].speedX -= x->mass[i].posX - tmpX;
                x->mass[i].speedY -= x->mass[i].posY - tmpY;
                x->mass[i].posX = tmpX;
                x->mass[i].posY = tmpY;
            }            

            // velocity damping of every masse (set a new force)
            x->mass[i].forceX = -x->mass[i].D2 * x->mass[i].speedX;
            x->mass[i].forceY = -x->mass[i].D2 * x->mass[i].speedY;

            // offset on velocity damping (to impose a specific velocity)
            speed = sqrt(x->mass[i].speedX * x->mass[i].speedX + x->mass[i].speedY * x->mass[i].speedY);
            if (speed != 0) {
                x->mass[i].forceX += x->mass[i].D2offset * (x->mass[i].speedX/speed);
                x->mass[i].forceY += x->mass[i].D2offset * (x->mass[i].speedY/speed);
            }
        }
    }
    for (i=0; i<x->nb_link; i++) // compute link forces
	{
        x->link[i].forceX = 0;
        x->link[i].forceY = 0;
 
        if ((x->link[i].active > 0) && (x->link[i].lType < 3))
        {
            Lx = x->link[i].mass1->posX - x->link[i].mass2->posX;
            Ly = x->link[i].mass1->posY - x->link[i].mass2->posY;
            L = sqrt( sqr(Lx) + sqr(Ly) );
        
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
                
                if (x->link[i].lType == 1)
                { // on projette selon 1 axe
                    // F = Fx*x->link[i].VX + Fy*x->link[i].VY; // produit scalaire de la force sur le vecteur qui la porte
                    Fx = Fx*x->link[i].VX; // V est unitaire, dc on projete sans pb
                    Fy = Fy*x->link[i].VY;                
                }
            
                x->link[i].mass1->forceX -= Fx;
                x->link[i].mass1->forceY -= Fy;
                x->link[i].mass2->forceX += Fx;
                x->link[i].mass2->forceY += Fy;
				x->link[i].forceX = Fx; // save for latter use
				x->link[i].forceY = Fy;
            }
            x->link[i].distance=L;
        }
        else if ((x->link[i].active > 0) && (x->link[i].lType == 3)) // hinge
        {
            L = getAngle(x, x->link[i].mass1->num, x->link[i].mass2->num, x->link[i].mass3->num);
            L = mod2Pi(L);
            if ( (L >= x->link[i].Lmin) && (L < x->link[i].Lmax)  && (L != 0))
            {
                F  = x->link[i].D *  (mod2Pi(L - x->link[i].distance)); 
                F += x->link[i].K *  pow_ch( mod2Pi(L - x->link[i].L), x->link[i].Pow); // calcule du couple

                // post("f=%f", mod2Pi(L - x->link[i].L));
                Dist = distance(x, x->link[i].mass1->num, x->link[i].mass2->num);
                tmp = F/Dist; // couple pour la mass 1
                Fx = tmp * (x->link[i].mass1->posY - x->link[i].mass2->posY)/Dist;
                Fy = tmp * (x->link[i].mass2->posX - x->link[i].mass1->posX)/Dist; // projection sur la normal a la liaison mass1 mass2
                x->link[i].mass1->forceX += Fx;
                x->link[i].mass1->forceY += Fy;
                x->link[i].mass2->forceX -= Fx;
                x->link[i].mass2->forceY -= Fy;
                
				x->link[i].forceX = Fx; // save for latter use
				x->link[i].forceY = Fy; // que doit on faire ds ce cas la : quel est la valeur a sauver ds la cas d'un couple???
                
                Dist = distance(x, x->link[i].mass3->num, x->link[i].mass2->num);
                F /= Dist;// pour la mass 2
                Fx = F * (x->link[i].mass3->posY - x->link[i].mass2->posY)/Dist;
                Fy = F * (x->link[i].mass2->posX - x->link[i].mass3->posX)/Dist; // projection sur la normal a la liaison mass2 mass3
                x->link[i].mass3->forceX -= Fx;
                x->link[i].mass3->forceY -= Fy;
                x->link[i].mass2->forceX += Fx;
                x->link[i].mass2->forceY += Fy;
            }
            x->link[i].distance=L; // sauvegarde de l'angle de la liaison
        }
    }
}

void pmpd2d_mass(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
// t_symbol *Id, t_float mobile, t_float M, t_float posX, t_float posY)
{ // add a mass  
	
 	if ( x->nb_mass >= x->nb_max_mass) {
 		x->nb_mass = x->nb_max_mass-1; 
 		pd_error(x, "pmpd2d masses number exceeded, please increase max masses number");
 	}

    x->mass[x->nb_mass].Id = gensym("mass");
	if ((argc > 0) &&  (argv[0].a_type == A_SYMBOL))
		x->mass[x->nb_mass].Id = atom_getsymbolarg(0,argc,argv);
	x->mass[x->nb_mass].mobile = 1;
	if ((argc > 1) &&  (argv[1].a_type == A_FLOAT))
		x->mass[x->nb_mass].mobile = (int) atom_getfloatarg(1, argc, argv);
	t_float M = 1;
	if ((argc > 2) &&  (argv[2].a_type == A_FLOAT))
		M = atom_getfloatarg(2, argc, argv);
	if (M<=0) M=1;
	x->mass[x->nb_mass].invM = 1/M;
	x->mass[x->nb_mass].speedX = 0;
	x->mass[x->nb_mass].speedY = 0;
	x->mass[x->nb_mass].posX = 0;
	if ((argc > 3) &&  (argv[3].a_type == A_FLOAT))
		x->mass[x->nb_mass].posX = atom_getfloatarg(3, argc, argv);
	x->mass[x->nb_mass].posY = 0;
	if ((argc > 4) &&  (argv[4].a_type == A_FLOAT))
		x->mass[x->nb_mass].posY = atom_getfloatarg(4, argc, argv);
	x->mass[x->nb_mass].forceX = 0;
	x->mass[x->nb_mass].forceY = 0;
	x->mass[x->nb_mass].num = x->nb_mass;
	x->mass[x->nb_mass].D2 = 0;
	x->mass[x->nb_mass].D2offset = 0;
	x->mass[x->nb_mass].overdamp = 0;
	x->nb_mass++ ;
}  
    
void pmpd2d_create_link(t_pmpd2d *x, t_symbol *Id, int mass1, int mass2, t_float K, t_float D, t_float Pow, t_float Lmin, t_float Lmax, t_int type)
{ // create a link based on mass number


    // post("%d,%d, K:%f, D:%f, Pow:%f, Lmin:%f, Lmax:%f", mass1, mass2, K, D, Pow, Lmin, Lmax);

    if ((x->nb_mass>1) && (mass1 != mass2) && (mass1 >= 0) && (mass2 >= 0) && (mass1 < x->nb_mass) && (mass2 < x->nb_mass) )
    {
		if ( x->nb_link >= x->nb_max_link) {
 			x->nb_link = x->nb_max_link-1; 
 			pd_error(x, "pmpd2d links number exceeded, please increase max links number");
 		}

        x->link[x->nb_link].lType = type;
        x->link[x->nb_link].Id = Id;
        x->link[x->nb_link].active = 1;
        x->link[x->nb_link].mass1 = &x->mass[mass1]; 
        x->link[x->nb_link].mass2 = &x->mass[mass2];
        x->link[x->nb_link].K = K;
        x->link[x->nb_link].D = D;
        x->link[x->nb_link].L = sqrt(sqr(x->mass[mass1].posX - x->mass[mass2].posX) + sqr(x->mass[mass1].posY - x->mass[mass2].posY));
        x->link[x->nb_link].Pow = Pow;
        x->link[x->nb_link].Lmin = Lmin;
        x->link[x->nb_link].Lmax = Lmax;
        x->link[x->nb_link].distance = x->link[x->nb_link].L ;
        x->link[x->nb_link].forceX = 0 ;
        x->link[x->nb_link].forceY = 0 ;

        x->nb_link++ ;
    }
}

void pmpd2d_link(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{ // add a link : Id, *mass1, *mass2, K, D, Pow, Lmin, Lmax;

    int i, j;
	t_symbol *Id = gensym("link");
	if ((argc > 0) &&  (argv[0].a_type == A_SYMBOL))
		Id = atom_getsymbolarg(0,argc,argv);
	t_float K = 0;
   	if ((argc > 3) &&  (argv[3].a_type == A_FLOAT))
		K = atom_getfloatarg(3, argc, argv);
	t_float D = 0;
   	if ((argc > 4) &&  (argv[4].a_type == A_FLOAT))
		D = atom_getfloatarg(4, argc, argv);
    t_float Pow = 1; 
    if ((argc > 5)  &&  (argv[5].a_type == A_FLOAT))
		Pow = atom_getfloatarg(5, argc, argv);
    t_float Lmin = -1000000;
    if ((argc > 6)  &&  (argv[6].a_type == A_FLOAT))
		Lmin = atom_getfloatarg(6, argc, argv);
    t_float Lmax =  1000000;
    if ((argc > 7)  &&  (argv[7].a_type == A_FLOAT))
		Lmax = atom_getfloatarg(7, argc, argv);
//    post("%d,%d, %f,%f", mass1, mass2, K, D);

    if ( ( argc > 2 ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) )
    {
        pmpd2d_create_link(x, Id, atom_getfloatarg(1, argc, argv), atom_getfloatarg(2, argc, argv), K, D, Pow, Lmin, Lmax, 0);
    }
    else if ( ( argc > 2 ) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                pmpd2d_create_link(x, Id, i, atom_getfloatarg(2, argc, argv), K, D, Pow, Lmin, Lmax, 0);
            }
        }
    }
    else if ( ( argc > 2 ) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(2,argc,argv) == x->mass[i].Id)
            {
                pmpd2d_create_link(x, Id, atom_getfloatarg(1, argc, argv), i, K, D, Pow, Lmin, Lmax, 0);
            }
        }
    }
    else if ( ( argc > 2 ) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_SYMBOL ) )
    {
        for (i=0; i < x->nb_mass; i++)
        {
            for (j=0; j < x->nb_mass; j++)
            {
                if ( (atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)&&(atom_getsymbolarg(2,argc,argv) == x->mass[j].Id))
                {
                    if (!( (x->mass[i].Id == x->mass[j].Id) && (i>j) )) 
                        // si lien entre 2 serie de masses identique entres elle, alors on ne creer qu'un lien sur 2, pour evider les redondances
                        pmpd2d_create_link(x, Id, i, j, K, D, Pow, Lmin, Lmax, 0);
                }
            }   
        }
    }
    else
		pmpd2d_create_link(x, Id, 0, 1, K, D, Pow, Lmin, Lmax, 0);
}

void pmpd2d_tLink(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{ // add a link : Id, *mass1, *mass2, K, D, Vx, Vy, Pow, Lmin, Lmax;

    int i, j;
     	
    t_symbol *Id = atom_getsymbolarg(0,argc,argv);
    int mass1 = atom_getfloatarg(1, argc, argv);
    int mass2 = atom_getfloatarg(2, argc, argv);
    t_float K = atom_getfloatarg(3, argc, argv);
    t_float D = atom_getfloatarg(4, argc, argv);
    t_float vecteurX = atom_getfloatarg(5, argc, argv);
    t_float vecteurY = atom_getfloatarg(6, argc, argv);
    t_float vecteur = sqrt( sqr(vecteurX) + sqr(vecteurY) );
    vecteurX /= vecteur;
    vecteurY /= vecteur;
    t_float Pow = 1; 
    if (argc > 7) Pow = atom_getfloatarg(7, argc, argv);
    t_float Lmin = 0;
    if (argc > 8) Lmin = atom_getfloatarg(8, argc, argv);
    t_float Lmax =  1000000;
    if (argc > 9) Lmax = atom_getfloatarg(9, argc, argv);

    if ( (argc > 6) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) )
    {
        pmpd2d_create_link(x, Id, mass1, mass2, K, D, Pow, Lmin, Lmax, 1);
        x->link[x->nb_link-1].VX = vecteurX;
        x->link[x->nb_link-1].VY = vecteurY;
    }
    else if ( (argc > 6) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                pmpd2d_create_link(x, Id, i, mass2, K, D, Pow, Lmin, Lmax, 1);
                x->link[x->nb_link-1].VX = vecteurX;
                x->link[x->nb_link-1].VY = vecteurY;
            }
        }
    }
    else if ( (argc > 6) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(2,argc,argv) == x->mass[i].Id)
            {
                pmpd2d_create_link(x, Id, mass1, i, K, D, Pow, Lmin, Lmax, 1);
                x->link[x->nb_link-1].VX = vecteurX;
                x->link[x->nb_link-1].VY = vecteurY;
            }
        }
    }
    else if ( (argc > 6) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_SYMBOL ) )
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
                        pmpd2d_create_link(x, Id, i, j, K, D, Pow, Lmin, Lmax, 1);
                        x->link[x->nb_link-1].VX = vecteurX;
                        x->link[x->nb_link-1].VY = vecteurY;
                    }

                }
            }   
        }
    }
}

void pmpd2d_tabLink(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
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

    if ( (argc > 5) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_FLOAT ) )
    {
        pmpd2d_create_link(x, Id, mass1, mass2, 1, 1, 1, 0, 1000000, 2);
        x->link[x->nb_link-1].arrayK = arrayK;
        x->link[x->nb_link-1].arrayD = arrayD;
        x->link[x->nb_link-1].K_L = Kl;
        x->link[x->nb_link-1].D_L = Dl;    
    }
    else if ( (argc > 5) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_FLOAT ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(1,argc,argv) == x->mass[i].Id)
            {
                pmpd2d_create_link(x, Id, i, mass2, 1, 1, 1, 0, 1000000, 2);
                x->link[x->nb_link-1].arrayK = arrayK;
                x->link[x->nb_link-1].arrayD = arrayD;
                x->link[x->nb_link-1].K_L = Kl;
                x->link[x->nb_link-1].D_L = Dl;    
            }
        }
    }
    else if ( (argc > 5) && ( argv[1].a_type == A_FLOAT ) && ( argv[2].a_type == A_SYMBOL ) )
    {
        for (i=0; i< x->nb_mass; i++)
        {
            if ( atom_getsymbolarg(2,argc,argv) == x->mass[i].Id)
            {
                pmpd2d_create_link(x, Id, mass1, i, 1, 1, 1, 0, 1000000, 2);
                x->link[x->nb_link-1].arrayK = arrayK;
                x->link[x->nb_link-1].arrayD = arrayD;
                x->link[x->nb_link-1].K_L = Kl;
                x->link[x->nb_link-1].D_L = Dl;    
            }
        }
    }
    else if ( (argc > 5) && ( argv[1].a_type == A_SYMBOL ) && ( argv[2].a_type == A_SYMBOL ) )
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
                        pmpd2d_create_link(x, Id, i, j, 1, 1, 1, 0, 1000000, 2);
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

void pmpd2d_hinge(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{ // TODO : acept symbol ID for hinge creation
    int mass1, mass2, mass3;
    
    x->link[x->nb_link].Id = gensym("hinge");
    if ((argc > 0) &&  (argv[0].a_type == A_SYMBOL))
		 x->link[x->nb_link].Id = atom_getsymbolarg(0,argc,argv);

    mass1 = 0;    
    if ((argc > 1) &&  (argv[1].a_type == A_FLOAT))
        mass1 = atom_getfloatarg(1, argc, argv);

    mass2 = 0;    
    if ((argc > 2) &&  (argv[2].a_type == A_FLOAT))
        mass2 = atom_getfloatarg(2, argc, argv);

    mass3 = 0;    
    if ((argc > 3) &&  (argv[3].a_type == A_FLOAT))
        mass3 = atom_getfloatarg(3, argc, argv);

    if ( (mass1 != mass2) && (mass2 != mass3) && (mass1 != mass3) \
        && (mass1 < x->nb_mass) && (mass2 < x->nb_mass) && (mass3 < x->nb_mass))
    {
		
		if ( x->nb_link >= x->nb_max_link) {
 			x->nb_link = x->nb_max_link-1; 
 			pd_error(x, "pmpd2d links number exceeded, please increase max links number");
 		}
		
		x->link[x->nb_link].K = 0;    
		if ((argc > 4) &&  (argv[4].a_type == A_FLOAT))
			x->link[x->nb_link].K = atom_getfloatarg(4, argc, argv);
    
		x->link[x->nb_link].D = 0;    
		if ((argc > 5) &&  (argv[5].a_type == A_FLOAT))
			x->link[x->nb_link].D = atom_getfloatarg(5, argc, argv);
         
		x->link[x->nb_link].active = 1;
		x->link[x->nb_link].Pow = 1;
		x->link[x->nb_link].Lmin = -4;
		x->link[x->nb_link].Lmax = 4;
    
        x->link[x->nb_link].mass1 = &x->mass[mass1];
        x->link[x->nb_link].mass2 = &x->mass[mass2];
        x->link[x->nb_link].mass3 = &x->mass[mass3];
        
        x->link[x->nb_link].L = getAngle(x, mass1, mass2, mass3); // angle actuel : -pi a pi
    
        x->link[x->nb_link].distance = x->link[x->nb_link].L; // angle au repos = angle a l'initialisation
        
        x->link[x->nb_link].lType = 3;
        
        x->nb_link++ ;
    }
}

void pmpd2d_delLink_int(t_pmpd2d *x, int dellink)
{
	int i;
	if ( ( dellink < x->nb_link ) && ( dellink >= 0) )
	{
		x->nb_link--;
		for (i=dellink; i < x->nb_link; i++)
		x->link[i]=x->link[i+1]; 
	}
}

void pmpd2d_delLink(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
	int i,nb_toremove;
	if ( (argc > 0) && ( argv[0].a_type == A_FLOAT ) )
		pmpd2d_delLink_int(x, atom_getfloatarg(0, argc, argv));
	if ( (argc > 0) && ( argv[0].a_type == A_SYMBOL ) )
/*		for (i=0; i<x->nb_link; )
			if ( atom_getsymbolarg(0,argc,argv) == x->link[i].Id )
				pmpd2d_delLink_int(x, i);
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

void pmpd2d_delMass_int(t_pmpd2d *x, int delmass)
{
	int i,nb_toremove;

	if ( ( delmass < x->nb_mass ) && ( delmass >= 0) )
	{
	/*	for (i=0; i < x->nb_link; ) // delete link connected to the mass to delete
		{
			if ( (x->link[i].mass1->num == delmass) || (x->link[i].mass2->num == delmass) )
			pmpd2d_delLink_int(x, i);
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
			if (x->link[i].mass2->num > delmass )
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

void pmpd2d_delMass(t_pmpd2d *x, t_symbol *s, int argc, t_atom *argv)
{
	int i, j, delmass, nb_toremove;
	if ( (argc > 0) && ( argv[0].a_type == A_FLOAT ) )
		pmpd2d_delMass_int(x, atom_getfloatarg(0, argc, argv));
	if ( (argc > 0) && ( argv[0].a_type == A_SYMBOL ) )
	/*
		for (i=0; i<x->nb_mass; )
			if ( atom_getsymbolarg(0,argc,argv) == x->mass[i].Id )
				pmpd2d_delMass_int(x, i);
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
						else if (x->link[j].mass2->num == i )
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
