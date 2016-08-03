/*
 *  pmpd2d.h
 *  
 */

#define max(a,b) ( ((a) > (b)) ? (a) : (b) ) 
#define min(a,b) ( ((a) < (b)) ? (a) : (b) ) 
    
static t_class *pmpd2d_class;

typedef struct _mass {
    t_symbol *Id;
    int mobile;
    t_float invM;
    t_float speedX;
    t_float speedY;
    t_float posX;
    t_float posY;
    t_float forceX;
    t_float forceY;
    t_float D2;
    t_float D2offset;
    t_float overdamp;
    int num;
} massStruct, *massPtr;

typedef struct _link {
    t_symbol *Id;
    int lType; // 0 pour un lien normal, 1 pour un tLink, 2 pour un tabLink, 3 pour un hinge
    struct _mass *mass1;
    struct _mass *mass2;
    struct _mass *mass3; // seulement pour le hinge
    t_int active;
    t_float K;
    t_float D;
    t_float L; // teta actuel
    t_float Pow;
    t_float Lmin; // tetamin pour un hinge
    t_float Lmax; // tetamax 
    t_float distance; // angle force nul de la charniere ds le cas d'un hinge
    t_float VX; // vecteur portant la liaison, si c'est le cas
    t_float VY;
    t_symbol *arrayK;
    t_symbol *arrayD;
    t_float K_L; // longeur du tabeau K
    t_float D_L; // longeur du tabeau D
    t_float forceX;
    t_float forceY;
} linkStruct, *linkPtr;

typedef struct _pmpd2d {
    t_object  x_obj;
    linkPtr link;
    massPtr mass;
    t_outlet *main_outlet;
    t_outlet *info_outlet;
    int nb_link;
    int nb_mass;
   	t_int nb_max_link;
	t_int nb_max_mass;
    t_float minX, maxX, minY, maxY;
    t_int grab; // si on grab une mass ou pas
    t_int grab_nb; // la masse grabé
} t_pmpd2d;

