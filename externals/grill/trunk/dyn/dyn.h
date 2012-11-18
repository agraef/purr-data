/* 
dyn - dynamical object management

Copyright (c)2003-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#ifndef __DYN_H
#define __DYN_H


#define DYN_VERSION_MAJOR 0
#define DYN_VERSION_MINOR 2

#define DYN_VERSION (DYN_VERSION_MAJOR*100+DYN_VERSION_MINOR)


#ifdef _MSC_VER
    #ifdef DYN_EXPORTS
        #define DYN_EXPORT __declspec(dllexport)
    #else
        #define DYN_EXPORT __declspec(dllimport)
    #endif
#else
    #define DYN_EXPORT
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* This is needed to correctly import external data */
#if defined(_WIN32) && !defined(NT)
#define NT
#endif

/* include PD public header for some type definitions */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4091 4244)
#endif
#include "m_pd.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif


#define DYN_ID_NONE 0


/* error codes */
#define DYN_ERROR_NONE 0  /* no error -> OK! */

#define DYN_ERROR_GENERAL -1
#define DYN_ERROR_TYPEMISMATCH -2
#define DYN_ERROR_NOTFOUND -3
#define DYN_ERROR_NOTCREATED -4  /* error creating object */

#define DYN_ERROR_PARAMS -101  /* wrong parameters */
#define DYN_ERROR_NOOBJ -102  /* object not found */
#define DYN_ERROR_NOSUB -103  /* sub-patcher not found */
#define DYN_ERROR_NOIN -104  /* inlet out of range */
#define DYN_ERROR_NOOUT -105  /* outlet out of range */

#define DYN_ERROR_CONN_GENERAL -200 /* could not connect/disconnect */
#define DYN_ERROR_CONN_PATCHER -201 /* objects to connect have different owners */


/*! Constants for message scheduling */
#define DYN_SCHED_AUTO -1
#define DYN_SCHED_NOW 1
#define DYN_SCHED_QUEUE 0


/* object types */
#define DYN_TYPE_PATCHER 1  /* object is a (sub-)patcher */
#define DYN_TYPE_OBJECT 2  /* object is a real object */
#define DYN_TYPE_MESSAGE 3  /* object is a message object */
#define DYN_TYPE_TEXT 4  /* object is text comment */
#define DYN_TYPE_CONN 5  /* object is a connection */
#define DYN_TYPE_LISTENER 6  /* object is a listener to an outlet */


/* inlet/outlet types */
#define DYN_INOUT_MESSAGE 0  /* message inlet/outlet */
#define DYN_INOUT_SIGNAL 1	/* signal inlet/outlet */

/* callback signals */
#define DYN_SIGNAL_NEW 0    /* object has been created */
#define DYN_SIGNAL_FREE 1   /* object has been destroyed */
#define DYN_SIGNAL_DISCONN 2   /* object has been disconnected */


/*! Type definition for any dyn object identifier */
struct dyn_ident;
typedef dyn_ident *dyn_id;


/*! Type of an object enumeration function 
    \param id Object ID
    \return 0 -> stop enumeration, != 0 -> go on
*/
typedef int dyn_enumfun(dyn_id id,void *data);

/* Type of an object callback function
    \param id Object id
    \param signal Type of signal (DYN_SIGNAL_* constant)
    \param data User defined data, passed at object creation
*/
typedef void dyn_callback(dyn_id id,int signal,void *data);

/* Type of a listener function
    \param lid Listener object ID
    \param id Object ID
    \param outlet Outlet index
    \param sym Message tag
    \param argc Message atom count
    \param argv Message atom list
*/
typedef void dyn_listener(dyn_id lid,dyn_id id,int outlet,const t_symbol *sym,int argc,const t_atom *argv,void *data);



/*! Get dyn version number
    \return version number major*100+minor
*/
DYN_EXPORT int dyn_Version();

/*! Restrict dyn operation to the calling thread
    \return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_Lock();

/*! Unrestrict dyn operation
    \return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_Unlock();


/*! Get number of pending scheduled operations
    \return number of commands, error code < 0
*/
DYN_EXPORT int dyn_Pending();


/*! Wait for all pending operations to finish
    \return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_Finish();


/*! Clear all objects
    \return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_Reset();


/*! Enumerate all objects in a patcher
    \param pid Patcher object ID
    \param fun Enumeration function
    \param data User defined data
    \return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_EnumObjects(dyn_id pid,dyn_enumfun fun,void *data);


/*! Create a new sub-patcher 
    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Pointer to new object id (returned on success)
    \param cb Callback function
    \param pid Pointer to parent patcher where the new object shall be created
    \return ok = 0, error code < 0

    \note If this command is queued, the object may not have been created on function return 
*/
DYN_EXPORT int dyn_NewPatcher(int sched,dyn_id *id,dyn_callback cb,dyn_id pid);


/*! Create a new object
    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Pointer to new object id (returned on success)
    \param cb Callback function
    \param pid Pointer to parent patcher where the new object shall be created
    \param obj Object name (symbol tag)
    \param argc Number of argument atoms
    \param argv Array of atoms
    \return ok = 0, error code < 0

    \note If this command is queued, the object may not have been created on function return 
*/
DYN_EXPORT int dyn_NewObject(int sched,dyn_id *id,dyn_callback cb,dyn_id pid,const t_symbol *obj,int argc,const t_atom *argv);

/*! Create a new object (with string command line)
    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Pointer to new object id (returned on success)
    \param cb Callback function
    \param pid Pointer to parent patcher where the new object shall be created
    \param args String of object name and arguments
    \return ok = 0, error code < 0

    \note If this command is queued, the object may not have been created on function return 
*/
DYN_EXPORT int dyn_NewObjectStr(int sched,dyn_id *id,dyn_callback cb,dyn_id pid,const char *args);


/*! Create a new message object
    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Pointer to new object id (returned on success)
    \param cb Callback function
    \param pid Pointer to parent patcher where the new object shall be created
    \param argc Number of argument atoms
    \param argv Array of atoms
    \return ok = 0, error code < 0

    \note If this command is queued, the object may not have been created on function return 
*/
DYN_EXPORT int dyn_NewMessage(int sched,dyn_id *id,dyn_callback cb,dyn_id pid,int argc,t_atom *argv);

/*! Create a new message object (with string message)
    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Pointer to new object id (returned on success)
    \param cb Callback function
    \param pid Pointer to parent patcher where the new object shall be created
    \param msg String of message text
    \return ok = 0, error code < 0

    \note If this command is queued, the object may not have been created on function return 
*/
DYN_EXPORT int dyn_NewMessageStr(int sched,dyn_id *id,dyn_callback cb,dyn_id pid,const char *msg);


/*! Connect two dyn objects
    \param sched Scheduling constant (DYN_SCHED_*)
    \param id return pointer to newly created connection object
    \param cb Callback function
    \param sid Source Object id (already present in dyn)
    \param outlet Source outlet index
    \param did Drain Object id (already present in dyn)
    \param outlet Drain inlet index
    \return ok = 0, error code < 0

    \note If this command is queued, the object may not have been created on function return 
*/
DYN_EXPORT int dyn_NewConnection(int sched,dyn_id *id,dyn_callback cb,dyn_id sid,int outlet,dyn_id did,int inlet);


/*! Delete a formerly created dyn object
    \note This can be any dyn object of type DYN_TYPE_*
    \note The ID will become invalid at function call

    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Object id (already present in dyn)
    \return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_Free(int sched,dyn_id id);


/*! Attach a data packet to a dyn object
    \param id Object ID (already present and valid)
    \param data Pointer to data packet 
	\return ok = 0, error code < 0

    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_SetData(dyn_id id,void *data);


/*! Retrieve a data packet from a dyn object
    \param id Object ID (already present and valid)
    \param data Pointer to data packet 
	\return ok = 0, error code < 0

    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_GetData(dyn_id id,void **data);


/*! Retrieve the object type
    \param id Object ID (already present and valid)
	\return object type (DYN_TYPE_*) >= 0, error code < 0

    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_GetType(dyn_id id);


/*! Get number of inlets 
    \param id Object ID (already present and valid)
	\return Number of inlets >= 0, error code < 0
    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_GetInletCount(dyn_id id);

/*! Get number of outlets 
    \param id Object ID (already present and valid)
	\return Number of outlets >= 0, error code < 0
    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_GetOutletCount(dyn_id id);


/*! Get inlet type
    \param id Object ID (already present and valid)
    \param inlet Inlet index
	\return Inlet type >= 0 (DYN_INOUT_MESSAGE or DYN_INOUT_SIGNAL), error code < 0
    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_GetInletType(dyn_id id,int inlet);

/*! Get outlet type
    \param id Object ID (already present and valid)
    \param outlet Outlet index
	\return Outlet type >= 0 (DYN_INOUT_MESSAGE or DYN_INOUT_SIGNAL), error code < 0
    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_GetOutletType(dyn_id id,int outlet);


/*! Enumerate connections to inlet
    \param id Object ID (already present and valid)
    \param inlet Inlet index
    \param fun Enumeration function
    \param data User data
	\return ok = 0, error code < 0
    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_EnumInletConnections(dyn_id id,int inlet,dyn_enumfun fun,void *data);

/*! Enumerate connections from outlet
    \param id Object ID (already present and valid)
    \param outlet Outlet index
    \param fun Enumeration function
    \param data User data
	\return ok = 0, error code < 0
    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_EnumOutletConnections(dyn_id id,int outlet,dyn_enumfun fun,void *data);


/*! Get source object of a connection
    \param cid Connection object ID
    \param oid Return pointer for connected object ID (may be NULL if not needed)
    \param outlet Return int for outlet index (may be NULL if not needed)
	\return ok = 0, error code < 0
    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_GetConnectionSource(dyn_id cid,dyn_id *oid,int *outlet);

/*! Get drain object of a connection
    \param cid Connection object ID
    \param oid Return pointer for connected object ID (may be NULL if not needed)
    \param inlet Return int for inlet index (may be NULL if not needed)
	\return ok = 0, error code < 0
    \note This is executed immediately without scheduling
*/
DYN_EXPORT int dyn_GetConnectionDrain(dyn_id cid,dyn_id *oid,int *inlet);


/*! Send a message to a dyn object
    \remark This often avoids the need for a connection to the respective object

    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Object ID
    \param inlet Inlet index to send message to
    \param sym Message tag
    \param argc Message atom count
    \param argv Message atom list
	\return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_Send(int sched,dyn_id id,int inlet,const t_symbol *sym,int argc,const t_atom *argv);

/*! Send a message (as a string) to a dyn object
    \remark This often avoids the need for a connection to the respective object

    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Object ID
    \param inlet Inlet index to send message to
    \param msg Message string
	\return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_SendStr(int sched,dyn_id id,int inlet,const char *msg);

/*! Listen to the outlet of a dyn object
    \remark This often avoids the need for a connection to the respective object

    \param sched Scheduling constant (DYN_SCHED_*)
    \param id Backpointer to created listener object ID
    \param oid Object ID to listen to
    \param outlet Outlet index to listen to
    \param cb Listener callback function
    \param data User data
	\return ok = 0, error code < 0
*/
DYN_EXPORT int dyn_Listen(int sched,dyn_id *id,dyn_id oid,int outlet,dyn_listener cb,void *data);


#ifdef __cplusplus
}
#endif


#endif /* __DYN_H */
