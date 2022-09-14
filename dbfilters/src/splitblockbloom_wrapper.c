#include <Python.h>
#include <stdio.h>
#include <stdbool.h>

#include "splitblockbloom.h"


static PyObject* method_construct_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* filename;
    uint32_t maxkeys = 0;
    double overfactor = 1.315;

    static char *kwlist[] = {"filename", "maxkeys", "overfactor", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|Id", kwlist, 
                                     &filename, &maxkeys, &overfactor)) 
        return NULL;
    
    return PyBool_FromLong(splitblockbloom_create(filename, maxkeys, overfactor));
}

static PyObject *method_query_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* pass;
    bool hashed = false;

    static char *kwlist[] = {"password", "hashed", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|b", kwlist, &pass, &hashed)) 
        return NULL;
    
    return PyBool_FromLong(splitblockbloom_query(pass, hashed));
}

static PyObject* method_sanity_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* filename;
    uint32_t maxkeys = 0;

    static char *kwlist[] = {"filename", "maxkeys", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|I", kwlist, 
                                     &filename, &maxkeys)) 
        return NULL;

    return PyBool_FromLong(splitblockbloom_sanity(filename, maxkeys));
}

static PyObject *method_fp_filter(PyObject *self, PyObject *args)
{
    uint32_t nkeys;

    if (!PyArg_ParseTuple(args, "I", &nkeys)) 
        return NULL;

    return PyLong_FromUnsignedLong(splitblockbloom_fp(nkeys));
}

static PyObject *method_save_filter(PyObject *self, PyObject *args)
{
    char* destfile;

    if (!PyArg_ParseTuple(args, "s", &destfile)) 
        return NULL;

    return PyBool_FromLong(splitblockbloom_save(destfile));
}

static PyObject *method_load_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* sourcefile;
    uint32_t maxkeys = 0;
    double overfactor = 1.315;

    static char *kwlist[] = {"sourcefile", "maxkeys", "overfactor", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|Id", kwlist, 
                                     &sourcefile, &maxkeys, &overfactor)) 
        return NULL;

    return PyBool_FromLong(splitblockbloom_load(sourcefile, maxkeys, overfactor));
}

static PyObject *method_exist_filter(PyObject *self, PyObject *args)
{
    return PyBool_FromLong(splitblockbloom_exist());
}

static PyObject *method_destroy_filter(PyObject *self, PyObject *args)
{
    splitblockbloom_destroy();
    Py_RETURN_NONE;
}


static PyMethodDef SplitblockbloomMethods[] =
{
    {"construct_filter", (PyCFunction) method_construct_filter, METH_VARARGS | METH_KEYWORDS, ""},
    {"query_filter", (PyCFunction) method_query_filter, METH_VARARGS | METH_KEYWORDS, ""},
    {"sanity_check", (PyCFunction) method_sanity_check, METH_VARARGS | METH_KEYWORDS, ""},
    {"fp_filter", (PyCFunction) method_fp_filter, METH_VARARGS, ""},
    {"save_filter", (PyCFunction) method_save_filter, METH_VARARGS, ""},
    {"load_filter", (PyCFunction) method_load_filter, METH_VARARGS | METH_KEYWORDS, ""},
    {"exist_filter", (PyCFunction) method_exist_filter, METH_NOARGS, ""},
    {"destroy_filter", (PyCFunction) method_destroy_filter, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef SplitblockbloomModule = 
{
    PyModuleDef_HEAD_INIT,
    "splitblockbloom",
    "",
    -1,
    SplitblockbloomMethods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_splitblockbloom(void) 
{
    //assert(!(__builtin_cpu_supports("avx2"))); //meter excepcion
    return PyModule_Create(&SplitblockbloomModule);
}