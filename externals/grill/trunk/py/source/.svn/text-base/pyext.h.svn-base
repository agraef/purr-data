/* 
py/pyext - python external object for PD and MaxMSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#ifndef __PYEXT_H
#define __PYEXT_H

#include "pybase.h"

class pyext
    : public pybase
    , public flext_dsp
{
	FLEXT_HEADER_S(pyext,flext_dsp,Setup)

public:
	pyext(int argc,const t_atom *argv,bool sig = false);

	static PyObject *pyext__str__(PyObject *,PyObject *args);

	static PyObject *pyext_outlet(PyObject *,PyObject *args);
#if FLEXT_SYS == FLEXT_SYS_PD
	static PyObject *pyext_tocanvas(PyObject *,PyObject *args);
#endif

	static PyObject *pyext_setattr(PyObject *,PyObject *args);
	static PyObject *pyext_getattr(PyObject *,PyObject *args);

	static PyObject *pyext_detach(PyObject *,PyObject *args);
	static PyObject *pyext_stop(PyObject *,PyObject *args);
	static PyObject *pyext_isthreaded(PyObject *,PyObject *);

	static PyObject *pyext_inbuf(PyObject *,PyObject *args);
	static PyObject *pyext_invec(PyObject *,PyObject *args);
	static PyObject *pyext_outbuf(PyObject *,PyObject *args);
	static PyObject *pyext_outvec(PyObject *,PyObject *args);

	int Inlets() const { return inlets; }
	int Outlets() const { return outlets; }

	static pyext *GetThis(PyObject *self);

protected:

    virtual bool Init();
    virtual bool Finalize();
    virtual void Exit();

    virtual bool CbMethodResort(int n,const t_symbol *s,int argc,const t_atom *argv);
    virtual void CbClick();
    virtual bool CbDsp();

    virtual void DumpOut(const t_symbol *sym,int argc,const t_atom *argv);

	bool work(int n,const t_symbol *s,int argc,const t_atom *argv); 

    void m_help();

    void m_reload() { Reload(); }
    void m_reload_(int argc,const t_atom *argv) { initargs(argc,argv); Reload(); }
    void ms_initargs(const AtomList &a) { m_reload_(a.Count(),a.Atoms()); }
    void m_dir_() { m__dir(pyobj); }
    void mg_dir_(AtomList &lst) { GetDir(pyobj,lst); }
    void m_doc_() { m__doc(((PyInstanceObject *)pyobj)->in_class->cl_dict); }

	void m_get(const t_symbol *s);
	void m_set(int argc,const t_atom *argv);

	const t_symbol *methname;
	PyObject *pyobj;
	int inlets,outlets;
    int siginlets,sigoutlets;

    flext::AtomList initargs;

	virtual void LoadModule();
	virtual void UnloadModule();

	virtual void Load();
	virtual void Unload();

	virtual bool DoInit();
	virtual void DoExit();

    virtual PyObject *GetSig(int ix,bool in);

private:
	static void Setup(t_classid);

	void SetThis();
	void ClearThis();

	void ClearBinding();
	bool MakeInstance();
    bool InitInOut(int &inlets,int &outlets);

	static PyObject *class_obj,*class_dict;
	static PyMethodDef attr_tbl[],meth_tbl[];
	static const char *pyext_doc;

	// -------- bind stuff ------------------
	static PyObject *pyext_bind(PyObject *,PyObject *args);
	static PyObject *pyext_unbind(PyObject *,PyObject *args);

	// ---------------------------

	bool call(const char *meth,int inlet,const t_symbol *s,int argc,const t_atom *argv);

    virtual void callpy(PyObject *fun,PyObject *args);
    static bool stcallpy(PyObject *fun,PyObject *args);

#ifndef PY_USE_GIL
	ThrState pythr;
#endif

private:
    static bool boundmeth(flext_base *,t_symbol *sym,int argc,t_atom *argv,void *data);

    FLEXT_CALLBACK(m_help)

    FLEXT_CALLBACK(m_reload)
	FLEXT_CALLBACK_V(m_reload_)
	FLEXT_CALLBACK(m_dir_)
	FLEXT_CALLGET_V(mg_dir_)
	FLEXT_CALLBACK(m_doc_)

    FLEXT_ATTRGET_V(initargs)
    FLEXT_CALLSET_V(ms_initargs)

	FLEXT_CALLBACK_S(m_get)
	FLEXT_CALLBACK_V(m_set)

	// callbacks
	FLEXT_ATTRVAR_I(detach)
	FLEXT_ATTRVAR_B(pymsg)
	FLEXT_ATTRVAR_B(respond)

	FLEXT_CALLBACK_V(m_stop)
	FLEXT_CALLBACK(m_dir)
	FLEXT_CALLGET_V(mg_dir)
	FLEXT_CALLBACK(m_doc)

	FLEXT_CALLBACK(CbClick)

#ifdef FLEXT_THREADS
    FLEXT_CALLBACK_T(tick)
#endif
};

#endif
