/* Copyright (C) 2007 L. Donnie Smith <cwiid@abstrakraft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ChangeLog:
 *  2007-08-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added py_wiimote_deinit
 *
 *  2007-06-28 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * supress error for nonexistent python plugins
 *
 *  2007-06-18 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * revised error messages
 *
 *  2007-06-05 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * relocated all python plugin logic here
 *  * now imports plugins without changing directories
 *
 *  2007-06-03 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added WMPLUGIN_ABS and WMPLUGIN_REL constants
 *
 *  2007-06-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 */

#include "Python.h"

#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

#include "structmember.h"
#include "cwiid.h"

#include "wmplugin.h"
#include "conf.h"
#include "util.h"

/* TODO: print error messages */
/* TODO: improve error checking */

struct py_plugin {
	PyObject *PyInfo;
	PyObject *handle;
	PyObject *init;
	PyObject *exec;
};

static PyObject *PyCWiidModule = NULL;
static PyObject *PySysModule = NULL;
static PyObject *PyPath = NULL;
static PyObject *PyWiimote = NULL;
static PyObject *(*ConvertMesgArray)(int, union cwiid_mesg[]);

static int py_plugin_info(struct plugin *, PyObject *);
static PyObject *set_rpt_mode(PyObject *, PyObject *, PyObject *);

#define WMPLUGIN_CONST_MACRO(a) {#a, WMPLUGIN_##a}
static struct {
	char *name;
	int value;
} wmplugin_constants[] = {
	WMPLUGIN_CONST_MACRO(ABS),
	WMPLUGIN_CONST_MACRO(REL),
	WMPLUGIN_CONST_MACRO(PARAM_INT),
	WMPLUGIN_CONST_MACRO(PARAM_FLOAT),
	{NULL, 0}
};

static PyMethodDef Module_Methods[] = 
{
	{"set_rpt_mode", (PyCFunction)set_rpt_mode, METH_VARARGS | METH_KEYWORDS,
	 "set_rpt_mode(id, rpt_mode)\n\nset the plugin report mode"},
	{NULL, NULL, 0, NULL}
};

int py_init(void)
{
	PyObject *PyObj, *PyWmPluginModule;
	int i;

	Py_InitializeEx(0);

	if (!(PyCWiidModule = PyImport_ImportModule("cwiid"))) {
		PyErr_Print();
		goto ERR_HND;
	}

	if (!(PySysModule = PyImport_ImportModule("sys"))) {
		PyErr_Print();
		goto ERR_HND;
	}

	if (!(PyPath = PyObject_GetAttrString(PySysModule, "path"))) {
		PyErr_Print();
		goto ERR_HND;
	}

	if (!(PyObj = PyObject_GetAttrString(PyCWiidModule,
	                                      "ConvertMesgArray"))) {
		PyErr_Print();
		goto ERR_HND;
	}
	ConvertMesgArray = PyCObject_AsVoidPtr(PyObj);
	Py_DECREF(PyObj);

	/* note: PyWmPluginModule is a borrowed reference - do not decref */
	if (!(PyWmPluginModule = Py_InitModule3("wmplugin", Module_Methods,
	                                        "wminput plugin interface"))) {
		PyErr_Print();
		goto ERR_HND;
	}

	for (i = 0; wmplugin_constants[i].name; i++) {
		if (PyModule_AddIntConstant(PyWmPluginModule,
		                            wmplugin_constants[i].name,
		                            wmplugin_constants[i].value)) {
			PyErr_Print();
			goto ERR_HND;
		}
	}

	return 0;

ERR_HND:
	if (PyCWiidModule) {
		Py_DECREF(PyCWiidModule);
		PyCWiidModule = NULL;
	}

	if (PyPath) {
		Py_DECREF(PyPath);
		PyPath = NULL;
	}

	if (PySysModule) {
		Py_DECREF(PySysModule);
		PySysModule = NULL;
	}

	Py_Finalize();

	return -1;
}

int py_wiimote(cwiid_wiimote_t *wiimote)
{
	PyObject *PyWiimoteType, *PyCObject, *PyArgs;

	if (!(PyWiimoteType = PyObject_GetAttrString(PyCWiidModule, "Wiimote"))) {
		PyErr_Print();
		return -1;
	}

	if (!(PyCObject = PyCObject_FromVoidPtr(wiimote, NULL))) {
		PyErr_Print();
		Py_DECREF(PyWiimoteType);
		return -1;
	}

	if (!(PyArgs = Py_BuildValue("(O)", PyCObject))) {
		PyErr_Print();
		Py_DECREF(PyCObject);
		Py_DECREF(PyWiimoteType);
		return -1;
	}

	Py_DECREF(PyCObject);

	if (!(PyWiimote = PyObject_CallObject(PyWiimoteType, PyArgs))) {
		PyErr_Print();
		Py_DECREF(PyArgs);
		Py_DECREF(PyWiimoteType);
		return -1;
	}

	Py_DECREF(PyArgs);
	Py_DECREF(PyWiimoteType);

	return 0;
}

void py_wiimote_deinit()
{
	Py_DECREF(PyWiimote);
}

void py_deinit(void)
{
	Py_DECREF(PyCWiidModule);
	Py_DECREF(PyPath);
	Py_DECREF(PySysModule);
	Py_Finalize();
}

int py_plugin_open(struct plugin *plugin, char *dir)
{
	PyObject *handle, *info;
	PyObject *PyStr;
	PyObject *PyErrType, *PyErr, *PyTraceback;

	if (!(PyStr = PyString_FromString(dir))) {
		PyErr_Print();
		return -1;
	}

	if (PyList_Insert(PyPath, 0, PyStr)) {
		Py_DECREF(PyStr);
		return -1;
	}

	if (!(handle = PyImport_ImportModule(plugin->name))) {
		/* ignore "module not found" errors in top level module */
		PyErr_Fetch(&PyErrType, &PyErr, &PyTraceback);
		PyErr_NormalizeException(&PyErrType, &PyErr, &PyTraceback);
		if (PyErr_GivenExceptionMatches(PyErr, PyExc_ImportError) &&
		  !PyTraceback) {
			Py_XDECREF(PyErrType);
			Py_XDECREF(PyErr);
		}
		else {
			PyErr_Restore(PyErrType, PyErr, PyTraceback);
			PyErr_Print();
		}

		if (PySequence_DelItem(PyPath, 0)) {
			PyErr_Print();
		}
		Py_DECREF(PyStr);
		return -1;
	}

	if (PySequence_DelItem(PyPath, 0)) {
		PyErr_Print();
	}
	Py_DECREF(PyStr);

	if (!(plugin->p = malloc(sizeof(struct py_plugin)))) {
		wminput_err("Error allocating py_plugin");
		return -1;
	}

	plugin->type = PLUGIN_PYTHON;
	plugin->info = NULL;
	plugin->data = NULL;
	((struct py_plugin *) plugin->p)->init = NULL;
	((struct py_plugin *) plugin->p)->exec = NULL;

	if (!(plugin->info = malloc(sizeof *plugin->info))) {
		wminput_err("Error allocating plugin info");
		goto ERR_HND;
	}
	if (!(plugin->data = malloc(sizeof *plugin->data))) {
		wminput_err("Error allocating plugin data");
		goto ERR_HND;
	}
	if (!(((struct py_plugin *)plugin->p)->init =
	  PyObject_GetAttrString(handle, "wmplugin_init"))) {
		PyErr_Print();
		goto ERR_HND;
	}
	if (!PyCallable_Check(((struct py_plugin *)plugin->p)->init)) {
		wminput_err("Unable to load plugin init function: not callable");
		goto ERR_HND;
	}
	if (!(((struct py_plugin *)plugin->p)->exec =
	  PyObject_GetAttrString(handle, "wmplugin_exec"))) {
		PyErr_Print();
		goto ERR_HND;
	}
	if (!PyCallable_Check(((struct py_plugin *)plugin->p)->exec)) {
		wminput_err("Unable to load plugin exec function: not callable");
		goto ERR_HND;
	}
	if (!(info = PyObject_GetAttrString(handle, "wmplugin_info"))) {
		PyErr_Print();
		goto ERR_HND;
	}
	if (!PyCallable_Check(info)) {
		wminput_err("Unable to load plugin info function: not callable");
		Py_DECREF((PyObject *)info);
		goto ERR_HND;
	}
	if (py_plugin_info(plugin, info)) {
		wminput_err("Error on python_info");
		Py_DECREF((PyObject *)info);
		goto ERR_HND;
	}
	Py_DECREF((PyObject *)info);

	((struct py_plugin *) plugin->p)->handle = handle;

	return 0;

ERR_HND:
	if (plugin->info) {
		free(plugin->info);
	}
	if (plugin->data) {
		free(plugin->data);
	}
	if (plugin->p) {
		if (((struct py_plugin *)plugin->p)->init) {
			Py_DECREF(((struct py_plugin *)plugin->p)->init);
		}
		if (((struct py_plugin *)plugin->p)->exec) {
			Py_DECREF(((struct py_plugin *)plugin->p)->exec);
		}
		free(plugin->p);
	}
	Py_DECREF(handle);
	return -1;
}

void py_plugin_close(struct plugin *plugin)
{
	free(plugin->info);
	free(plugin->data);
	Py_DECREF(((struct py_plugin *)plugin->p)->PyInfo);
	Py_DECREF(((struct py_plugin *)plugin->p)->init);
	Py_DECREF(((struct py_plugin *)plugin->p)->exec);
	Py_DECREF(((struct py_plugin *)plugin->p)->handle);
	free(plugin->p);
}

static int py_plugin_info(struct plugin *plugin, PyObject *info)
{
	PyObject *PyButtonInfo, *PyAxisInfo, *PyParamInfo;
	PyObject *PyObj;
	int i;

	if (!(((struct py_plugin *)plugin->p)->PyInfo =
	  PyObject_CallObject(info, NULL))) {
		PyErr_Print();
		goto ERR_HND;
	}

	if (!PyArg_ParseTuple(((struct py_plugin *)plugin->p)->PyInfo, "OOO",
	                      &PyButtonInfo, &PyAxisInfo, &PyParamInfo)) {
		PyErr_Print();
		goto ERR_HND;
	}

	if (!(PySequence_Check(PyButtonInfo) && PySequence_Check(PyAxisInfo) &&
	      PySequence_Check(PyParamInfo))) {
		wminput_err("Type error in wminput_info: info not sequences");
		goto ERR_HND;
	}

	plugin->info->button_count = PySequence_Size(PyButtonInfo);
	for (i=0; i < plugin->info->button_count; i++) {
		if (!(PyObj = PySequence_GetItem(PyButtonInfo, i))) {
			PyErr_Print();
			goto ERR_HND;
		}

		if (!(plugin->info->button_info[i].name = PyString_AsString(PyObj))) {
			PyErr_Print();
			Py_DECREF(PyObj);
			goto ERR_HND;
		}

		Py_DECREF(PyObj);
	}

	plugin->info->axis_count = PySequence_Size(PyAxisInfo);
	for (i=0; i < plugin->info->axis_count; i++) {
		unsigned int type;

		if (!(PyObj = PySequence_GetItem(PyAxisInfo, i))) {
			PyErr_Print();
			goto ERR_HND;
		}

		if (!PyArg_ParseTuple(PyObj, "sIiiii",
		                      &plugin->info->axis_info[i].name,
		                      &type,
		                      &plugin->info->axis_info[i].max,
		                      &plugin->info->axis_info[i].min,
		                      &plugin->info->axis_info[i].fuzz,
		                      &plugin->info->axis_info[i].flat)) {
			PyErr_Print();
			Py_DECREF(PyObj);
			goto ERR_HND;
		}

		plugin->info->axis_info[i].type = type;

		Py_DECREF(PyObj);
	}

	plugin->info->param_count = PySequence_Size(PyParamInfo);
	for (i=0; i < plugin->info->param_count; i++) {
		if (!(PyObj = PySequence_GetItem(PyParamInfo, i))) {
			PyErr_Print();
			goto ERR_HND;
		}

		if (!PyArg_ParseTuple(PyObj, "siO", &plugin->info->param_info[i].name,
		                      &plugin->info->param_info[i].type,
		                      &plugin->info->param_info[i].ptr)) {
			PyErr_Print();
			Py_DECREF(PyObj);
			goto ERR_HND;
		}

		Py_DECREF(PyObj);
	}

	return 0;

ERR_HND:
	if (((struct py_plugin *)plugin->p)->PyInfo) {
		Py_DECREF(((struct py_plugin *)plugin->p)->PyInfo);
	}

	return -1;
}

int py_plugin_init(struct plugin *plugin, int id)
{
	PyObject *PyArgs;

	if (!(PyArgs = Py_BuildValue("(i,O)", id, PyWiimote))) {
		PyErr_Print();
		return -1;
	}

	if (!PyObject_CallObject(((struct py_plugin *)plugin->p)->init, PyArgs)) {
		PyErr_Print();
		Py_DECREF(PyArgs);
		return -1;
	}

	Py_DECREF(PyArgs);

	return 0;
}

int py_plugin_exec(struct plugin *plugin, int mesg_count,
                   union cwiid_mesg mesg[])
{
	PyObject *PyArgs, *PyMesg, *PyData, *PyButtonData, *PyAxisData, *PyObj;
	int i;

	if (!(PyMesg = ConvertMesgArray(mesg_count, mesg))) {
		PyErr_Print();
		return -1;
	}

	if (!(PyArgs = Py_BuildValue("(O)", PyMesg))) {
		PyErr_Print();
		Py_DECREF(PyMesg);
		return -1;
	}

	Py_DECREF(PyMesg);

	if (!(PyData = PyObject_CallObject(((struct py_plugin *)plugin->p)->exec,
	                                   PyArgs))) {
		PyErr_Print();
		Py_DECREF(PyArgs);
		return -1;
	}

	Py_DECREF(PyArgs);

	if (!PyArg_ParseTuple(PyData, "OO", &PyButtonData, &PyAxisData)) {
		PyErr_Print();
		Py_DECREF(PyData);
		return -1;
	}

	if (!(PySequence_Check(PyButtonData) && PySequence_Check(PyAxisData))) {
		wminput_err("Type error on wminput_exec: exec not sequences");
		Py_DECREF(PyData);
		return -1;
	}

	if (PySequence_Size(PyButtonData) != plugin->info->button_count) {
		wminput_err("Type error on wminput_exec: bad button sequence");
		Py_DECREF(PyData);
		return -1;
	}
	plugin->data->buttons = 0;
	for (i=0; i < plugin->info->button_count; i++) {
		if (!(PyObj = PySequence_GetItem(PyButtonData, i))) {
			PyErr_Print();
			Py_DECREF(PyData);
			return -1;
		}

		if (PyObj == Py_True) {
			plugin->data->buttons |= 1<<i;
		}
		else if (PyObj != Py_False) {
			wminput_err("Type error on wminput_exec: bad button value");
			Py_DECREF(PyObj);
			Py_DECREF(PyData);
			return -1;
		}

		Py_DECREF(PyObj);
	}

	if (PySequence_Size(PyAxisData) != plugin->info->axis_count) {
		wminput_err("Error on wminput_exec: bad axis sequence");
		Py_DECREF(PyData);
		return -1;
	}
	for (i=0; i < plugin->info->axis_count; i++) {
		if (!(PyObj = PySequence_GetItem(PyAxisData, i))) {
			PyErr_Print();
			Py_DECREF(PyData);
			return -1;
		}

		if (PyObj == Py_None) {
			plugin->data->axes[i].valid = 0;
		}
		else if (!PyInt_Check(PyObj)) {
			wminput_err("Type error on wminput_exec: bad axis value");
			Py_DECREF(PyObj);
			Py_DECREF(PyData);
			return -1;
		}
		else {
			plugin->data->axes[i].valid = 1;
			plugin->data->axes[i].value = PyInt_AsLong(PyObj);
		}

		Py_DECREF(PyObj);
	}

	Py_DECREF(PyData);

	return 0;
}

int py_plugin_param_int(struct plugin *plugin, int i, int value)
{
	PyObject *PyObj;

	switch (plugin->info->param_info[i].type) {
	case WMPLUGIN_PARAM_INT:
		PyObj = PyInt_FromLong(value);
		if (PyObject_SetAttrString(((struct py_plugin *)plugin->p)->handle,
		                           plugin->info->param_info[i].name,
		                           PyObj)) {
			PyErr_Print();
			return -1;
		}
		break;
	case WMPLUGIN_PARAM_FLOAT:
		PyObj = PyFloat_FromDouble((double)value);
		if (PyObject_SetAttrString(((struct py_plugin *)plugin->p)->handle,
		                           plugin->info->param_info[i].name,
		                           PyObj)) {
			PyErr_Print();
			return -1;
		}
		break;
	}

	return 0;
}

int py_plugin_param_float(struct plugin *plugin, int i, float value)
{
	PyObject *PyObj;

	switch (plugin->info->param_info[i].type) {
	case WMPLUGIN_PARAM_INT:
		wminput_err("possible loss of precision: %s.%s (cast float to int)",
		            plugin->name, plugin->info->param_info[i].name);
		PyObj = PyInt_FromLong((int)value);
		if (PyObject_SetAttrString(((struct py_plugin *)plugin->p)->handle,
		                           plugin->info->param_info[i].name,
		                           PyObj)) {
			PyErr_Print();
			return -1;
		}
		break;
	case WMPLUGIN_PARAM_FLOAT:
		PyObj = PyFloat_FromDouble((double)value);
		if (PyObject_SetAttrString(((struct py_plugin *)plugin->p)->handle,
		                           plugin->info->param_info[i].name,
		                           PyObj)) {
			PyErr_Print();
			return -1;
		}
		break;
	}

	return 0;
}

static PyObject *set_rpt_mode(PyObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = {"id", "rpt_mode", NULL};
	int id, rpt_mode;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii:wmplugin:set_rpt_mode",
	                                 kwlist, &id, &rpt_mode)) {
		return NULL;
	}

	if (wmplugin_set_rpt_mode(id, rpt_mode)) {
		return NULL;
	}

	Py_RETURN_NONE;
}
