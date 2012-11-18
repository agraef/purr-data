/* 
py/pyext - python external object for PD and MaxMSP

Copyright (c)2002-2011 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate: 2011-03-12 15:13:33 -0500 (Sat, 12 Mar 2011) $
$LastChangedBy: thomas $
*/

#include "pybase.h"
#include <map>

#if FLEXT_OS == FLEXT_OS_WIN
#include <windows.h>
#elif FLEXT_OS == FLEXT_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

static PyMethodDef StdOut_Methods[] =
{
	{ "write", pybase::StdOut_Write, 1 },
	{ NULL,    NULL,           }  
};

static PyObject *gcollect = NULL;

#ifdef FLEXT_THREADS

class ThrCmp
{
public:
    inline bool operator()(const flext::thrid_t &a,const flext::thrid_t &b) const
    {
        if(sizeof(a) == sizeof(size_t))
            return *(size_t *)&a < *(size_t *)&b;
        else
            return memcmp(&a,&b,sizeof(a)) < 0;
    }
};


int pybase::lockcount = 0;

#ifndef PY_USE_GIL
static PyInterpreterState *pymain = NULL;

typedef std::map<flext::thrid_t,ThrState,ThrCmp> PyThrMap;

static PyThrMap pythrmap;
ThrState pybase::pythrsys = NULL;

ThrState pybase::FindThreadState()
{
    flext::thrid_t id = flext::GetThreadId();
	PyThrMap::iterator it = pythrmap.find(id);
    if(it == pythrmap.end()) {
        // Make new thread state
        ThrState st = PyThreadState_New(pymain);
        pythrmap[id] = st;
        return st;
    }
    else 
        return it->second;
}

void pybase::FreeThreadState()
{
    flext::thrid_t id = flext::GetThreadId();
	PyThrMap::iterator it = pythrmap.find(id);
    if(it != pythrmap.end()) {
        // clear out any cruft from thread state object
        PyThreadState_Clear(it->second);
        // delete my thread state object
        PyThreadState_Delete(it->second);
        // delete from map
        pythrmap.erase(it);
    }
}
#endif // PY_USE_GIL

PyFifo pybase::qufifo;
flext::ThrCond pybase::qucond;
#endif


PyObject *pybase::module_obj = NULL;
PyObject *pybase::module_dict = NULL;

PyObject *pybase::builtins_obj = NULL;
PyObject *pybase::builtins_dict = NULL;

const t_symbol *pybase::sym_fint = NULL;
const t_symbol *pybase::sym_response = NULL;

// -----------------------------------------------------------------------------------------------------------


void initsymbol();
void initsamplebuffer();
void initbundle();

void pybase::lib_setup()
{   
    post("");
	post("------------------------------------------------");
	post("py/pyext %s - python script objects",PY__VERSION);
	post("(C)2002-2011 Thomas Grill - http://grrrr.org/ext");
    post("");
    post("using Python %s",Py_GetVersion());

#ifdef FLEXT_DEBUG
    post("");
	post("DEBUG version compiled on %s %s",__DATE__,__TIME__);
#endif

	// -------------------------------------------------------------

    sym_response = flext::MakeSymbol("response");

#if FLEXT_SYS == FLEXT_SYS_PD
    sym_fint = sym_float;
#else
    sym_fint = sym_int;
#endif

	// -------------------------------------------------------------

	Py_Initialize();

#ifdef FLEXT_DEBUG
//	Py_DebugFlag = 1;
//	Py_VerboseFlag = 1;
#else
    Py_OptimizeFlag = 1;
#endif

#ifdef FLEXT_THREADS
    // enable thread support and acquire the global thread lock
	PyEval_InitThreads();

#ifndef PY_USE_GIL
    // get thread state
    pythrsys = PyThreadState_Get();
    // get main interpreter state
	pymain = pythrsys->interp;

    // add thread state of main thread to map
    pythrmap[GetThreadId()] = pythrsys;
#endif // PY_USE_GIL

#endif

    // sys.argv must be set to empty tuple
    char *nothing = "";
	PySys_SetArgv(0,&nothing);

    // register/initialize pyext module only once!
	module_obj = Py_InitModule(PYEXT_MODULE, func_tbl);
	module_dict = PyModule_GetDict(module_obj); // borrowed reference

	PyModule_AddStringConstant(module_obj,"__doc__",(char *)py_doc);

	// redirect stdout
	PyObject* py_out;
    py_out = Py_InitModule("stdout", StdOut_Methods);
	PySys_SetObject("stdout", py_out);
    py_out = Py_InitModule("stderr", StdOut_Methods);
	PySys_SetObject("stderr", py_out);

    // get garbage collector function
    PyObject *gcobj = PyImport_ImportModule("gc");
    if(gcobj) {
        gcollect = PyObject_GetAttrString(gcobj,"collect");
        Py_DECREF(gcobj);
    }

    builtins_obj = PyImport_ImportModule("__builtin__");
	builtins_dict = PyModule_GetDict(builtins_obj); // borrowed reference

    // add symbol type
    initsymbol();
    PyModule_AddObject(module_obj,"Symbol",(PyObject *)&pySymbol_Type);

    // pre-defined symbols
    PyModule_AddObject(module_obj,"_s_",(PyObject *)pySymbol__);
    PyModule_AddObject(module_obj,"_s_bang",(PyObject *)pySymbol_bang);
    PyModule_AddObject(module_obj,"_s_list",(PyObject *)pySymbol_list);
    PyModule_AddObject(module_obj,"_s_symbol",(PyObject *)pySymbol_symbol);
    PyModule_AddObject(module_obj,"_s_float",(PyObject *)pySymbol_float);
    PyModule_AddObject(module_obj,"_s_int",(PyObject *)pySymbol_int);

    // add samplebuffer type
    initsamplebuffer();
    PyModule_AddObject(module_obj,"Buffer",(PyObject *)&pySamplebuffer_Type);

    // add message bundle type
    initbundle();
    PyModule_AddObject(module_obj,"Bundle",(PyObject *)&pyBundle_Type);

	// -------------------------------------------------------------
#if FLEXT_SYS == FLEXT_SYS_PD && defined(PD_DEVEL_VERSION) && defined(PY_USE_INOFFICIAL)
    // add PD paths

    char *dir;
    for(int i = 0; (dir = namelist_get(sys_searchpath,i)) != NULL; ++i) {
        AddToPath(dir);
    }
#endif
	// -------------------------------------------------------------

	FLEXT_SETUP(pyobj);
	FLEXT_SETUP(pymeth);
	FLEXT_SETUP(pyext);
#ifndef PY_NODSP
	FLEXT_DSP_SETUP(pydsp);
#endif

#ifdef FLEXT_THREADS
    // release global lock
    PyEval_ReleaseLock();

    // launch thread worker
    LaunchThread(quworker,NULL);

    // launch python worker
    LaunchThread(pyworker,NULL);
#endif

	post("------------------------------------------------");
    post("");
}

FLEXT_LIB_SETUP(py,pybase::lib_setup)


// -----------------------------------------------------------------------------------------------------------


pybase::pybase()
    : module(NULL)
    , dict(NULL)
#ifdef FLEXT_THREADS
    , thrcount(0)
    , shouldexit(false),stoptick(0)
#endif
	, detach(0)
    , pymsg(false)
{
    ThrLock lock;
	Py_INCREF(module_obj);
}

pybase::~pybase()
{
    ThrLock lock;
   	Py_XDECREF(module_obj);
}

void pybase::Exit()
{
#ifdef FLEXT_THREADS
    erasethreads();

    shouldexit = true;
    qucond.Signal();

    if(thrcount) {
		// Wait for a certain time
		for(int i = 0; i < (PY_STOP_WAIT/PY_STOP_TICK) && thrcount; ++i) 
            Sleep(PY_STOP_TICK*0.001f);
        if(thrcount) {
		    // Wait forever
		    post("py/pyext - Waiting for thread termination!");
		    while(thrcount) Sleep(PY_STOP_TICK*0.001f);
		    post("py/pyext - Okay, all threads have terminated");
        }
	}
#endif
}

void pybase::GetDir(PyObject *obj,AtomList &lst)
{
    if(obj) {
        ThrLock lock;
    
        PyObject *pvar  = PyObject_Dir(obj);
	    if(!pvar)
		    PyErr_Print(); // no method found
	    else {
            const t_symbol *sym = GetPyArgs(lst,pvar);
            if(!sym)
                post("py/pyext - Argument list could not be created");
            else
                FLEXT_ASSERT(sym == sym_list);
            Py_DECREF(pvar);
        }
    }
}

void pybase::m__dir(PyObject *obj)
{
    AtomList lst;
    GetDir(obj,lst);
    // dump dir to attribute outlet
    DumpOut(NULL,lst.Count(),lst.Atoms());
}

void pybase::m__doc(PyObject *obj)
{
    if(obj) {
        ThrLock lock;

		PyObject *docf = PyDict_GetItemString(obj,"__doc__"); // borrowed!!!
		if(docf && PyString_Check(docf)) {

			post("");
			const char *s = PyString_AS_STRING(docf);

			// FIX: Python doc strings can easily be larger than 1k characters
			// -> split into separate lines
			for(;;) {
				char *nl = strchr((char *)s,'\n'); // the cast is for Borland C++
				if(!nl) {
					// no more newline found
					post(s);
					break;
				}
				else {
                    char buf[1024];
					// copy string before newline to temp buffer and post
					unsigned int l = nl-s;
					if(l >= sizeof(buf)) l = sizeof buf-1;
					strncpy(buf,s,l); // copy all but newline
					buf[l] = 0;
					post(buf);
					s = nl+1;  // set after newline
				}
			}
		}
	}
}

void pybase::OpenEditor()
{
    if(!module) return;
    const char *mname = PyModule_GetFilename(module);
    if(!mname) {
        PyErr_Clear();
        return;
    }

    char fname[1024];
    strcpy(fname,mname);

    // replacing .pyc or .pyo for source file name
    char *dt = strrchr(fname,'.');
    if(dt && !strncmp(dt,".py",2)) strcpy(dt,".py");

    // this should open the editor....
#if FLEXT_OS == FLEXT_OS_WIN
    int err = (int)ShellExecute(NULL,"edit",fname,NULL,NULL,SW_SHOW);
    if(err == SE_ERR_NOASSOC) {
        // no association found - try notepad
        err = (int)ShellExecute(NULL,NULL,"notepad.exe",fname,NULL,SW_SHOW);
    }
    else if(err == ERROR_FILE_NOT_FOUND || err == SE_ERR_FNF)
        post("py/pyext - File not %s found",fname);
    else if(err <= 32)
        post("py/pyext - Unknown error opening %s",fname);
       
#elif FLEXT_OS == FLEXT_OS_MAC
	FSRef ref;
    OSStatus err = FSPathMakeRef((unsigned char *)fname,&ref,NULL);
    if(err)
        post("py/pyext - Error interpreting path %s",fname);
    else {
		FSRef editor;
		err = LSGetApplicationForItem(&ref,kLSRolesEditor,&editor,NULL);
        if(err) {
			// Can't find associated application... try Textedit
			err = FSPathMakeRef((unsigned char *)"/Applications/TextEdit.app",&editor,NULL);
			if(err)
				post("py/pyext - Can't find Textedit application");
		}
		
		if(!err) {
			LSLaunchFSRefSpec lspec;
			lspec.appRef = &editor;
			lspec.numDocs = 1;
			lspec.itemRefs = &ref;
			lspec.passThruParams = NULL;
			lspec.launchFlags = kLSLaunchDefaults;
			lspec.asyncRefCon = NULL;
			err = LSOpenFromRefSpec(&lspec,NULL);
			if(err)
				post("py/pyext - Couldn't launch editor");
		}
    }
#else
    // thanks to Tim Blechmann

    char *editor = getenv("EDITOR");

    if(!editor) { // || !strcmp(editor, "/usr/bin/nano") || !strcmp(editor, "/usr/bin/pico") || !strcmp(editor, "/usr/bin/vi")) {
        // no environment variable or console text editor found ... use idle instead (should have come with Python)
        editor = "idle";
    }

    pid_t child = fork();  
    if(!child) {
        char cmd[80];
        strcpy(cmd,editor);
        strcat(cmd," ");
        strcat(cmd,fname);
        execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);
    }
#endif
}

void pybase::SetArgs()
{
	// script arguments
    int argc = args.Count();
    const t_atom *argv = args.Atoms();
	char **sargv = new char *[argc+1];
	for(int i = 0; i <= argc; ++i) {
		sargv[i] = new char[256];
		if(!i) 
			strcpy(sargv[i],"py/pyext");
		else
			GetAString(argv[i-1],sargv[i],255);
	}

	// the arguments to the module are only recognized once! (at first use in a patcher)
	PySys_SetArgv(argc+1,sargv);

	for(int j = 0; j <= argc; ++j) delete[] sargv[j];
	delete[] sargv;
}

static bool getmodulesub(const char *mod,char *dir,int len,char *ext)
{
#if FLEXT_SYS == FLEXT_SYS_PD
	char *name;
	int fd = open_via_path("",mod,ext,dir,&name,len,0);
    if(fd > 0) {
        FLEXT_ASSERT(name && *name);
        close(fd);
    }
    else {
        // look for mod/__init__.py
        std::string tmp(mod); 
        int l = tmp.size();
        tmp += "/__init__";
        fd = open_via_path("",tmp.c_str(),ext,dir,&name,len,0);
        if(fd > 0) {
            FLEXT_ASSERT(name && *name);
            close(fd);
            // we must remove the module name from dir
            char *t = dir+strlen(dir)-l;
            FLEXT_ASSERT(!strcmp(mod,t) && t[-1] == '/');
            t[-1] = 0;
        }
        else
            name = NULL;
    }

	// if dir is current working directory... name points to dir
	if(dir == name) strcpy(dir,".");
    return name != NULL;
#elif FLEXT_SYS == FLEXT_SYS_MAX
    short path;
    long type;
    char smod[1024];
    strcpy(smod,mod);
    strcat(smod,ext);
    bool ok = !locatefile_extended(smod,&path,&type,&type,0);
    if(ok)
        // convert pathname to unix style
        path_topathname(path,NULL,smod);
    else {
        // do path/file.ext combinations work at all under Max?
        strcpy(smod,mod);

        short path;
        type = 'fold';
        ok = !locatefile_extended(smod,&path,&type,&type,1);
        if(ok) {
            // convert pathname to unix style (including trailing slash)
            path_topathname(path,NULL,smod);
            char *end = smod+strlen(smod);
            strcpy(end,mod);
            strcat(end,"/__init__");
            strcat(end,ext);

            // check if file is really existing: try to open it
            FILE *f = fopen(smod,"r");
            if(f) {
                *end = 0;  // clear module part ... we only need base path
                fclose(f);
            }
            else
                ok = false;
        }
    }
    
    if(ok) {
        // convert path into slash style needed for Python
#if 0
		// Max API function uses Volume:/Path notation
        path_nameconform(smod,dir,PATH_STYLE_SLASH,PATH_TYPE_ABSOLUTE);
#else
#if FLEXT_OS == FLEXT_OS_WIN
        char *colon = NULL;
#else
        char *colon = strchr(smod,':');
#endif
        if(colon) {
            *colon = 0;
            strcpy(dir,"/Volumes/");
            strcat(dir,smod);
            strcat(dir,colon+1);
        }
        else
            strcpy(dir,smod);
#endif
        return true;
    }
    else
        // not found
        return false;
#else
#pragma message("Not implemented");
    return false;
#endif
}

static bool getmodulepath(const char *mod,char *dir,int len)
{
    return 
        getmodulesub(mod,dir,len,".py") ||
        getmodulesub(mod,dir,len,".pyc") ||
        getmodulesub(mod,dir,len,".pyo");
}

bool pybase::ImportModule(const char *name)
{
    if(name) {
        if(modname == name) {
            // module with the same name is already loaded
            if(module) return true;
        }
        else
            modname = name;
    }
    else
        modname.clear();

    UnimportModule();
    return ReloadModule();
}

void pybase::UnimportModule()
{
    if(module) {
        FLEXT_ASSERT(dict && module_obj && module_dict);

	    Py_DECREF(module);

	    // reference count to module is not 0 here, altough probably the last instance was unloaded
	    // Python retains one reference to the module all the time 
	    // we don't care

	    module = NULL;
	    dict = NULL;
    }
}

bool pybase::ReloadModule()
{
    SetArgs();
    PyObject *newmod;
    
    if(modname.length()) {

        if(module)
            newmod = PyImport_ReloadModule(module);
        else {
            // search in module path (TODO: check before if module is already present to avoid costly searching)
            char dir[1024];
	        if(getmodulepath(modname.c_str(),dir,sizeof(dir)))
    	        AddToPath(dir);
//            else
//                PyErr_SetString(PyExc_ImportError,"Module not found in path");

#if 1
            // strip off eventual subpath from module name
            // it should already have been considered in the above AddToPath call
            size_t p = modname.rfind('/');
            const char *m;
            if(p == std::string::npos)
                m = modname.c_str();
            else {
                // reuse dir buffer...
                strcpy(dir,modname.c_str()+p+1);
                m = dir;
            }

            // module could also be loaded ok, even if it's not in the path (e.g. for internal stuff)
            newmod = PyImport_ImportModule((char *)m);
#else
            newmod = PyImport_ImportModule((char *)modname.c_str());
#endif
        }
    }
    else {
        // if no module name given, take py module
        newmod = module_obj; 
        Py_INCREF(newmod);
    }

	if(!newmod) {
		// unload faulty module
        UnimportModule();
        return false;
	}
	else {
		Py_XDECREF(module);
		module = newmod;
		dict = PyModule_GetDict(module); // borrowed
        return true;
	}
}

void pybase::AddToPath(const char *dir)
{
	if(dir && *dir) {
		PyObject *pobj = PySys_GetObject("path");
		if(pobj && PyList_Check(pobj)) {
    		PyObject *ps = PyString_FromString(dir);
            if(!PySequence_Contains(pobj,ps))
				PyList_Append(pobj,ps); // makes new reference
            Py_DECREF(ps);
		}
		PySys_SetObject("path",pobj); // steals reference to pobj
	}
}

void pybase::AddCurrentPath(flext_base *o)
{
	char dir[1024];

    // add dir of current patch to path
    o->GetCanvasDir(dir,sizeof(dir));
	if(*dir) AddToPath(dir);

	// add current dir to path
#if FLEXT_SYS == FLEXT_SYS_PD
	AddToPath(GetString(canvas_getcurrentdir()));
#elif FLEXT_SYS == FLEXT_SYS_MAX
    short path = path_getdefault();
	path_topathname(path,NULL,dir);
	AddToPath(dir);
#endif
}

bool pybase::OutObject(flext_base *ext,int o,PyObject *obj)
{
    flext::AtomListStatic<16> lst;
    const t_symbol *sym = pymsg?GetPyAtom(lst,obj):GetPyArgs(lst,obj);
    if(sym) {
        // call to outlet _outside_ the Mutex lock!
        // otherwise (if not detached) deadlock will occur
        ext->ToOutAnything(o,sym,lst.Count(),lst.Atoms());
        return true;
    }
    else
        return false;
}

void pybase::Reload()
{
	ThrLock lock;

	PyObject *reg = GetRegistry(REGNAME);

    if(reg) {
        PyObject *key;
        Py_ssize_t pos = 0;
        while(PyDict_Next(reg,&pos,&key,NULL)) {
            pybase *th = (pybase *)PyLong_AsLong(key);
            FLEXT_ASSERT(th);
            th->Unload();
        }

        UnloadModule();
    }

	bool ok = ReloadModule();

    if(ok) {
        LoadModule();

        if(reg) {
            SetRegistry(REGNAME,reg);

            PyObject *key;
            Py_ssize_t pos = 0;
            while(PyDict_Next(reg,&pos,&key,NULL)) {
                pybase *th = (pybase *)PyLong_AsLong(key);
                FLEXT_ASSERT(th);
                th->Load();
            }
        }
        else
            Load();
    }

    Report();
}

static PyObject *output = NULL;

// post to the console
PyObject* pybase::StdOut_Write(PyObject* self, PyObject* args)
{
    // should always be a tuple
    FLEXT_ASSERT(PyTuple_Check(args));

	const int sz = PyTuple_GET_SIZE(args);

	for(int i = 0; i < sz; ++i) {
		PyObject *val = PyTuple_GET_ITEM(args,i); // borrowed reference
		PyObject *str = PyObject_Str(val); // new reference
		char *cstr = PyString_AS_STRING(str);
		char *lf = strchr(cstr,'\n');

		// line feed in string
		if(!lf) {
			// no -> just append
            if(output)
				PyString_ConcatAndDel(&output,str); // str is decrefd
			else
				output = str; // take str reference
		}
		else {
			// yes -> append up to line feed, reset output buffer to string remainder
			PyObject *part = PyString_FromStringAndSize(cstr,lf-cstr); // new reference
            if(output)
				PyString_ConcatAndDel(&output,part); // str is decrefd	
			else
				output = part; // take str reference

            // output concatenated string
			post(PyString_AS_STRING(output));

			Py_DECREF(output);
			output = PyString_FromString(lf+1);  // new reference
		}
	}

    Py_INCREF(Py_None);
    return Py_None;
}


class work_data
{
public:
    work_data(PyObject *f,PyObject *a): fun(f),args(a) {}
    ~work_data() { Py_DECREF(fun); Py_DECREF(args); }

    PyObject *fun,*args;
};

bool pybase::gencall(PyObject *pmeth,PyObject *pargs)
{
	bool ret = false;

    // Now call method
    switch(detach) {
        case 0:
            ret = docall(pmeth,pargs);
        	Py_DECREF(pargs);
        	Py_DECREF(pmeth);
            break;
#ifdef FLEXT_THREADS
        case 1:
            // put call into queue
            ret = qucall(pmeth,pargs);
            break;
        case 2:
            // each call a new thread
            if(!shouldexit) {
                thr_params *p = new thr_params;
                p->cl = (flext_base *)this;
                p->var->_ext = new work_data(pmeth,pargs);
			    ret = LaunchThread(thrworker,p);
			    if(!ret) post("py/pyext - Failed to launch thread!");
		    }
            break;
#endif
        default:
            post("py/pyext - Unknown detach mode");
    }
    return ret;
}

void pybase::exchandle()
{
#if 0
    // want to use that, but exception keeps a reference to the object
    // might be a Python bug!
    PyErr_Print();
#else
    // must use that instead... clear the exception
    PyObject *type,*value,*traceback;
    PyErr_Fetch(&type,&value,&traceback);
    PyErr_NormalizeException(&type,&value,&traceback);
    PyErr_Display(type,value,traceback);

    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
#endif
}

#ifdef FLEXT_THREADS
void pybase::thrworker(thr_params *p)
{
    FLEXT_ASSERT(p);
    pybase *th = (pybase *)p->cl;
	work_data *w = (work_data *)p->var->_ext;

	++th->thrcount; // \todo this should be atomic
    {
        ThrLock lock;
    // call worker
	th->docall(w->fun,w->args);
	delete w;
    }
    --th->thrcount; // \todo this should be atomic
}

/*! This thread function basically keeps alive the Python interpreter in the background
	It's good for threads that have been started from scripted functions
*/
void pybase::pyworker(thr_params *)
{
	ThrLock lock;
	PyObject *timemod = PyImport_ImportModule("time");
	PyObject *sleep = PyObject_GetAttrString(timemod,"sleep");
	PyObject *args = PyTuple_New(1);
	PyTuple_SET_ITEM(args,0,PyFloat_FromDouble(1000000));

    for(;;) {
		PyObject *res = PyObject_CallObject(sleep,args);
		Py_DECREF(res);
	}
}

void pybase::quworker(thr_params *)
{
    FifoEl *el;
    ThrState my = FindThreadState(),state;

    for(;;) {
        while(el = qufifo.Get()) {
        	++el->th->thrcount; // \todo this should be atomic
            {
                ThrLock lock(my);
            el->th->docall(el->fun,el->args);
            Py_XDECREF(el->fun);
            Py_XDECREF(el->args);
            }
            --el->th->thrcount; // \todo this should be atomic
            qufifo.Free(el);
        }
        qucond.Wait();
    }

    // we never end
    if(false) {
        ThrLock lock(my);
    // unref remaining Python objects
    while(el = qufifo.Get()) {
        Py_XDECREF(el->fun);
        Py_XDECREF(el->args);
        qufifo.Free(el);
    }
}
}

void pybase::erasethreads()
{
    PyFifo tmp;
    FifoEl *el;
    while(el = qufifo.Get()) {
        if(el->th == this) {
            Py_XDECREF(el->fun);
            Py_XDECREF(el->args);
            qufifo.Free(el);
        }
        else
            tmp.Put(el);
    }
    // Push back
    while(el = tmp.Get()) qufifo.Put(el);
}
#endif

bool pybase::collect()
{
    if(gcollect) {
        PyObject *ret = PyObject_CallObject(gcollect,NULL);
        if(ret) {
#ifdef FLEXT_DEBUG
            int refs = PyInt_AsLong(ret);
            if(refs) post("py/pyext - Garbage collector reports %i unreachable objects",refs);
#endif
            Py_DECREF(ret);
            return false;
        }
    }
    return true;
}
