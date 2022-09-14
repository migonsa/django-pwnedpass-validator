#include <Python.h>
#include <stdio.h>
#include <stdbool.h>

#include "xor16.h"


static PyObject* method_construct_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* filename;
    uint32_t maxkeys = 0;

    static char *kwlist[] = {"filename", "maxkeys", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|I", kwlist, 
                                     &filename, &maxkeys)) 
        return NULL;
        
    return PyBool_FromLong(xor16_create(filename, maxkeys));
}

static PyObject *method_query_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* pass;
    bool hashed = false;

    static char *kwlist[] = {"password", "hashed", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|b", kwlist, &pass, &hashed)) 
        return NULL;

    return PyBool_FromLong(xor16_query(pass, hashed));
}

static PyObject* method_sanity_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* filename;
    uint32_t maxkeys = 0;

    static char *kwlist[] = {"filename", "maxkeys", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|I", kwlist, 
                                     &filename, &maxkeys)) 
        return NULL;

    return PyBool_FromLong(xor16_sanity(filename, maxkeys));
}

static PyObject *method_fp_filter(PyObject *self, PyObject *args)
{
    uint32_t nkeys;

    if (!PyArg_ParseTuple(args, "I", &nkeys)) 
        return NULL;

    return PyLong_FromUnsignedLong(xor16_fp(nkeys));
}

static PyObject *method_save_filter(PyObject *self, PyObject *args)
{
    char* destfile;

    if (!PyArg_ParseTuple(args, "s", &destfile)) 
        return NULL;

    return PyBool_FromLong(xor16_save(destfile));
}

static PyObject *method_load_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* sourcefile;
    uint32_t maxkeys = 0;

    static char *kwlist[] = {"sourcefile", "maxkeys", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|I", kwlist, 
                                     &sourcefile, &maxkeys)) 
        return NULL;

    return PyBool_FromLong(xor16_load(sourcefile, maxkeys));
}

static PyObject *method_exist_filter(PyObject *self, PyObject *args)
{
    return PyBool_FromLong(xor16_exist());
}

static PyObject *method_destroy_filter(PyObject *self, PyObject *args)
{
    xor16_destroy();
    Py_RETURN_NONE;
}


static PyMethodDef Xor16Methods[] =
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

static struct PyModuleDef Xor16Module = 
{
    PyModuleDef_HEAD_INIT,
    "xor16",
    "",
    -1,
    Xor16Methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_xor16(void) 
{
    //assert(!(__builtin_cpu_supports("avx2"))); //meter excepcion
    return PyModule_Create(&Xor16Module);
}