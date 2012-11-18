/* [psql] - A PostgreSQL client for PD
 *
 * Copyright (C) 2006 Jamie Bullock and others
 *
 * Large portions of the code are based on [sqlsingle] by Iain Mott
 *
 * Copyright (C) 2001 Iain Mott
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2, or (at your option) 
 * any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License, which should be included with this 
 * program, for more details. 
 * 
 */

/*  up to 10 fields may be returned. returns floats or symbols */

/* code for psql pd class */

#include "m_pd.h"
#include <string.h>

/* PSQL */

#include <stdio.h>
#include <stdlib.h>
#include "libpq-fe.h" 

#define MAXSQLFIELDS 20

/*  postgres datatypes and corresponding 'Oid's */

#define PGINT4 23
#define PGFLOAT8 701
#define PGDOUBLE 1700
#define PGDATE 1082  
#define PGDATETIME 1184
#define PGVARCHAR 1043

typedef struct psql{ 

    t_object t_ob;
    t_outlet *x_outlet1,
             *x_outlet2;
    t_atom get_atom;
    t_symbol *x_sym;
    char sqlStringStore[MAXPDSTRING];
    char *pghost,
         *pgport,
         *pgoptions,
         *pgtty,
         *dbName;
    char port[20];
    PGconn *conn;
    t_int connected,
          in_query;

} t_psql;

static void psql_SQL (t_psql *x, t_symbol *s){ 

    char sqlString[MAXPDSTRING];  
    int argc = 10;
    t_atom argv[argc];
    t_atom *test;
    int descriptor_fnum;
    int starttime_fnum;
    int endtime_fnum;
    int spurtorder_fnum;
    int       nFields;
    int       i, j;
    t_symbol *t_sym;
    PGresult   *res;
    int tupplecount;
    test = argv;

    /*    make a connection to the database */
    if(!x->connected){
        post("Reconnecting to database, %s", x->dbName);
        x->conn = PQsetdb(x->pghost, x->pgport, 
                x->pgoptions, x->pgtty, x->dbName); 
    }


    if(PQstatus(x->conn) == CONNECTION_BAD){
        fprintf(stderr, "Connection to database '%s' failed.\n", x->dbName);
        fprintf(stderr, "%s", PQerrorMessage(x->conn));
    }
    else{
        x->connected = 1;

        res = PQexec(x->conn, x->sqlStringStore);
        if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != 
                PGRES_COMMAND_OK)
        {
            fprintf(stderr, "psql: Action failed. PQresultStatus is %s\n", 
                    PQresStatus(PQresultStatus(res)));
        }

        else {
            tupplecount = PQntuples(res);
            if (!res || PQresultStatus(res) != PGRES_TUPLES_OK)
            {
                PQclear(res);
            }
            else {

                nFields = PQnfields(res);


                descriptor_fnum = PQfnumber(res, "descriptor");
                starttime_fnum = PQfnumber(res, "starttime");
                endtime_fnum = PQfnumber(res, "endtime");
                spurtorder_fnum = PQfnumber(res, "spurtorder");

                /* fetch the instances */
                for (i = 0; i < PQntuples(res); i++) {
                    /* merge field of a query instance into a list */
                    SETFLOAT(&argv[0], i);
                    for (j=0; j<nFields; j++)
                    {
                        int fType =  PQftype(res, j);

                        if (fType == PGINT4)
                            SETFLOAT(&argv[j+1], (float) atoi(PQgetvalue(res, i, j)));
                        else if (fType == PGFLOAT8 || fType == PGDOUBLE)
                            SETFLOAT(&argv[j+1], (float) atof(PQgetvalue(res, i, j)));
                        else if (fType == PGVARCHAR || fType == PGDATE || fType == PGDATETIME)
                        {
                            t_sym = gensym( PQgetvalue(res, i, j));
                            SETSYMBOL(&argv[j+1], t_sym);
                        }
                        else {
                            t_sym = gensym( PQgetvalue(res, i, j));
                            SETSYMBOL(&argv[j+1], t_sym);
                            post(
                    "Undefined PG data type. OID: %d. Stored in list as Symbol", fType);
                        }

                    }
                    t_sym = gensym( "A_FLOAT");
                    outlet_list(x->x_outlet1, t_sym, nFields+1, argv);
                }
                outlet_bang(x->x_outlet2);
                PQclear(res);
            }
        }
    }
}

static void psql_close(t_psql *x){
    post("Closing connection to database %s", x->dbName);
    PQfinish(x->conn);
    x->connected = 0;
}

static void psql_anything(t_psql *x, t_symbol *s, int ac, t_atom *av, t_floatarg f){ 
    char sqlString[MAXPDSTRING]; 
    int i;
    char buf[MAXPDSTRING];
    char mybuf[MAXPDSTRING];

    if(!strcmp(s->s_name,"close") && !x->in_query)
        psql_close(x);
    else{
        if (strcmp(s->s_name, "sql")){


            strcat(x->sqlStringStore,  ", ");

            /* replace the truncated first symbol */

            strcat(x->sqlStringStore,  s->s_name); 
            strcat(x->sqlStringStore, " ");

            /* see if it ends OK  */
            atom_string(av+ac-1, buf, MAXPDSTRING);

            if (!strcmp(buf, "sqlend")){

                int tc = ac-1;


                for (i = 0; i < tc; i++){

                    atom_string(av+i, buf, MAXPDSTRING);
                    strcat(x->sqlStringStore, buf);
                    if (i < tc - 1)
                        strcat(x->sqlStringStore, " ");

                }

                psql_SQL (x, s);

                x->in_query = 0;

            }
            else {
                for (i = 0; i < ac; i++)
                {
                    atom_string(av+i, buf, MAXPDSTRING);
                    strcat(x->sqlStringStore, buf);
                    if (i < ac - 1)
                        strcat(x->sqlStringStore, " ");
                }
            }

        }
        else {
            /*  if s->s_name DOES equal "sql" -  first clear sqlStringStore then check 
             *  if end of string terminates with "sqlend" */

            x->in_query = 1;

            strcpy(x->sqlStringStore, "");
            atom_string(av+ac-1, buf, MAXPDSTRING);
            if (strcmp(buf, "sqlend"))
            { 
                for (i = 0; i < ac; i++)
                {
                    atom_string(av+i, buf, MAXPDSTRING);
                    strcat(x->sqlStringStore, buf);
                    if (i < ac - 1)
                        strcat(x->sqlStringStore, " ");
                }
            }

            else 
            {
                ac -= 1;
                for (i = 0; i < ac; i++)
                {
                    atom_string(av+i, buf, MAXPDSTRING);
                    strcat(x->sqlStringStore, buf);
                    if (i < ac - 1)
                        strcat(x->sqlStringStore, " ");
                }
                psql_SQL (x, s);

                x->in_query = 0;
            }
        }

        atom_string(av+ac-1, buf, MAXPDSTRING);
    }
}

/*  this one is needed if the new line begins with a number */

static void psql_list(t_psql *x, t_symbol *s, int ac, t_atom *av)
{
    int i;
    char buf[MAXPDSTRING];

    strcat(x->sqlStringStore, ",");

    if (strcmp(x->sqlStringStore, "")){ 

        atom_string(av+ac-1, buf, MAXPDSTRING);
        if (strcmp(buf, "sqlend") == 0){ 

            ac = ac -1;

            for (i = 0; i < ac; i++){
                strcat(x->sqlStringStore, " ");
                atom_string(av+i, buf, MAXPDSTRING);
                strcat(x->sqlStringStore, buf);

            }

            x->in_query = 0;

        }
        else{

            for (i = 0; i < ac; i++){
                strcat(x->sqlStringStore, " ");
                atom_string(av+i, buf, MAXPDSTRING);
                strcat(x->sqlStringStore, buf);

            }
        }
    }
    else
        post("psql: Not SQL");

}

t_class *psql_class;

static void *psql_new(t_symbol *s, int argc, t_atom *argv)
{
    t_psql *x = (t_psql *)pd_new(psql_class);
    x->x_sym = gensym("psql");
    x->x_outlet1 = outlet_new(&x->t_ob, &s_list);
    x->x_outlet2 = outlet_new(&x->t_ob, &s_bang);

    x->in_query = 0;

    if(argc == 0)
    {
        x->pghost = NULL;      /* host name of the backend server */
        x->pgport = NULL;          /* port of the backend server */
        x->pgoptions = NULL;      /* special options to start up the backend server */
        x->pgtty = NULL;          /* debugging tty for the backend server */
        x->dbName = "template1";
        post("using dbase template1 on local UNIX socket");
    }
    else if(argc == 1 && argv[0].a_type == A_SYMBOL)
    {
        x->pghost = NULL;      /* host name of the backend server */
        x->pgport = NULL;          /* port of the backend server */
        x->pgoptions = NULL;      /* special options to start up the backend server */
        x->pgtty = NULL;          /* debugging tty for the backend server */
        x->dbName = argv[0].a_w.w_symbol->s_name;
    }
    else if(argc == 3 && argv[0].a_type == A_SYMBOL && argv[1].a_type == A_SYMBOL
            && argv[2].a_type == A_FLOAT)
    {
        x->pghost = argv[1].a_w.w_symbol->s_name;   /* host name of the backend server */
        sprintf(x->port, "%d", (int)argv[2].a_w.w_float);
        x->pgport =  x->port;          /* port of the backend server */
        //    strncpy(x->pgport, tmp);          /* port of the backend server */
        x->pgoptions = NULL;      /* special options to start up the backend server */
        x->pgtty = NULL;          /* debugging tty for the backend server */
        x->dbName = argv[0].a_w.w_symbol->s_name;
    }

    else
    {
        x->pghost = NULL;      /* host name of the backend server */
        x->pgport = NULL;          /* port of the backend server */
        x->pgoptions = NULL;      /* special options to start up the backend server */
        x->pgtty = NULL;          /* debugging tty for the backend server */
        x->dbName = "template1";
        post("psql: invalid arguments using default template1 dbase on localhost");
    }

    /* check postmaster is running on specified port and machine by attempting to 
     * x->connect to template1 */
    x->conn = PQsetdb(x->pghost, x->pgport, x->pgoptions, x->pgtty, "template1");
    if (PQstatus(x->conn) == CONNECTION_BAD)
    {
        fprintf(stderr, 
                "psql: Connection to template1 failed. Perhaps the postmaster is not running on the specified port and machine \n");
        fprintf(stderr, "psql: Connect error is: %s", PQerrorMessage(x->conn));
    }
    else
        psql_close(x);
    return (x);
}

static void psql_free(t_psql *x){
    if(x->connected)
        psql_close(x);
}

void psql_setup(void) {
    psql_class = class_new(gensym("psql"), (t_newmethod)psql_new, (t_method)psql_free,
            sizeof(t_psql),  0, A_GIMME, 0);
    class_addanything(psql_class, psql_anything);
    class_addlist(psql_class, psql_list);
}
