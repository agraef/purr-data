/* 
py/pyext - python script object for PD and Max/MSP

Copyright (c)2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate: 2008-04-13 08:14:54 -0400 (Sun, 13 Apr 2008) $
$LastChangedBy: thomas $
*/

#ifndef PY_NODSP

#include "pyext.h"

class pydsp
    : public pyext
{
	FLEXT_HEADER(pydsp,pyext)
public:
    pydsp(int argc,const t_atom *argv);

protected:
    virtual bool CbDsp();
    virtual void CbSignal();

    virtual bool DoInit();
    virtual void DoExit();

    virtual PyObject *GetSig(int ix,bool in);

    void NewBuffers();
    void FreeBuffers();

    PyObject *dspfun,*sigfun;
    PyObject **buffers;
};

FLEXT_LIB_DSP_V("pyext~ pyext.~ pyx~ pyx.~",pydsp)

pydsp::pydsp(int argc,const t_atom *argv)
    : pyext(argc,argv,true) 
    , dspfun(NULL),sigfun(NULL)
{}

bool pydsp::DoInit()
{
    if(!pyext::DoInit()) return false;
    
    if(pyobj) 
	{ 
        dspfun = PyObject_GetAttrString(pyobj,"_dsp"); // get ref
	    if(!dspfun)
			PyErr_Clear();
		else if(!PyMethod_Check(dspfun)) {
            Py_DECREF(dspfun);
		    dspfun = NULL;
		}
	}
    return true;
}

void pydsp::DoExit()
{
    if(dspfun) { Py_DECREF(dspfun); dspfun = NULL; }
    if(sigfun) { Py_DECREF(sigfun); sigfun = NULL; }

    FreeBuffers();
}

PyObject *arrayfrombuffer(PyObject *buf,int c,int n);

void pydsp::NewBuffers()
{
    int i,n = Blocksize();
    const int ins = CntInSig(),outs = CntOutSig();
    t_sample *const *insigs = InSig();
    t_sample *const *outsigs = OutSig();

    // inlet/outlet count can't change so we don't have to deallocate
    if(!buffers) {
        int cnt = ins+outs;
        buffers = new PyObject *[cnt];
        memset(buffers,0,cnt*sizeof(*buffers));
    }

    for(i = 0; i < ins; ++i) {
        Py_XDECREF(buffers[i]);
        PyObject *b = PyBuffer_FromReadWriteMemory(insigs[i],n*sizeof(t_sample));
        buffers[i] = arrayfrombuffer(b,1,n);
        Py_DECREF(b);
    }
    for(i = 0; i < outs; ++i) {
        Py_XDECREF(buffers[ins+i]);
        if(i < ins && outsigs[i] == insigs[i]) {
            // same vectors - share the objects!
            buffers[ins+i] = buffers[i];
            Py_XINCREF(buffers[i]);
        }
        else {
            PyObject *b = PyBuffer_FromReadWriteMemory(outsigs[i],n*sizeof(t_sample));
            buffers[ins+i] = arrayfrombuffer(b,1,n);
            Py_DECREF(b);
        }
    }
}

void pydsp::FreeBuffers()
{
    if(buffers) {
        int cnt = CntInSig()+CntOutSig();
        for(int i = 0; i < cnt; ++i) Py_XDECREF(buffers[i]);
        delete[] buffers;
        buffers = NULL;
    }
}

bool pydsp::CbDsp()
{
    if(pyobj && (CntInSig() || CntOutSig()))
    {
       	ThrLockSys lock;

        NewBuffers();

		bool dodsp = true;
        if(dspfun) {
            PyObject *ret = PyObject_CallObject(dspfun,NULL);
            if(ret) {
				dodsp = PyObject_IsTrue(ret) != 0;
                Py_DECREF(ret);
			}
            else {
#ifdef FLEXT_DEBUG
                PyErr_Print();
#else
                PyErr_Clear();
#endif   
            }
        }

        // do that here instead of where dspfun is initialized, so that
        // _signal can be assigned in _dsp
        // optimizations may be done there to assign the right _signal version
        Py_XDECREF(sigfun);
		
		if(dodsp) {
			sigfun = PyObject_GetAttrString(pyobj,"_signal"); // get ref
			if(!sigfun)
				PyErr_Clear();
			else if(!PyMethod_Check(sigfun)) {
				Py_DECREF(sigfun);
				sigfun = NULL;
			}
		}
		else
			sigfun = NULL;

        return sigfun != NULL;
    }
    else
        // switch on dsp only if there are signal inlets or outlets
        return false;
}

void pydsp::CbSignal()
{
    ThrLockSys lock;
    PyObject *ret = PyObject_CallObject(sigfun,NULL);

    if(ret) 
        Py_DECREF(ret);
    else {
#ifdef FLEXT_DEBUG
        PyErr_Print();
#else
        PyErr_Clear();
#endif   
    }
}

PyObject *pydsp::GetSig(int ix,bool in) 
{
    PyObject *r = buffers[in?ix:CntInSig()+ix];
    Py_XINCREF(r);
    return r;
}

#endif
