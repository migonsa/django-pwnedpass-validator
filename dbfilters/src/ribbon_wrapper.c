#include <Python.h>
#include <stdio.h>
#include <stdbool.h>

#include "ribbon128_avx2.h"


static PyObject* method_construct_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* filename;
    uint32_t maxkeys = 0;
    uint8_t rbytes = 1;
    double overfactor = 0.;

    static char *kwlist[] = {"filename", "maxkeys", "r", "overfactor", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|IBd", kwlist, 
                                     &filename, &maxkeys, &rbytes, &overfactor)) 
        return NULL;
        
    assert(rbytes == 1 || rbytes == 2);
    if(overfactor <= 0)
        overfactor = 1. + (4. + 2.*rbytes)/128.;

    return PyBool_FromLong(create_ribbon128(filename, maxkeys, rbytes, overfactor));
}

static PyObject *method_query_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* pass;
    bool hashed = false;

    static char *kwlist[] = {"password", "hashed", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|b", kwlist, &pass, &hashed)) 
        return NULL;

    return PyBool_FromLong(query_ribbon128(pass, hashed));
}

static PyObject* method_sanity_check(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* filename;
    uint32_t maxkeys = 0;

    static char *kwlist[] = {"filename", "maxkeys", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|I", kwlist, 
                                     &filename, &maxkeys)) 
        return NULL;

    return PyBool_FromLong(sanity_check(filename, maxkeys));
}

static PyObject *method_fp_filter(PyObject *self, PyObject *args)
{
    uint32_t nkeys;

    if (!PyArg_ParseTuple(args, "I", &nkeys)) 
        return NULL;

    return PyLong_FromUnsignedLong(fp_filter(nkeys));
}

static PyObject *method_save_filter(PyObject *self, PyObject *args)
{
    char* destfile;

    if (!PyArg_ParseTuple(args, "s", &destfile)) 
        return NULL;

    return PyBool_FromLong(save_filter(destfile));
}

static PyObject *method_load_filter(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* sourcefile;
    uint32_t maxkeys = 0;
    uint8_t rbytes = 1;
    double overfactor = 0.;

    static char *kwlist[] = {"sourcefile", "maxkeys", "r", "overfactor", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|IBd", kwlist, 
                                     &sourcefile, &maxkeys, &rbytes, &overfactor)) 
        return NULL;
    
    assert(rbytes == 1 || rbytes == 2);
    if(overfactor <= 0)
        overfactor = 1. + (4. + 2.*rbytes)/128.;

    return PyBool_FromLong(load_filter(sourcefile, maxkeys, rbytes, overfactor));
}

static PyObject *method_exist_filter(PyObject *self, PyObject *args)
{
    return PyBool_FromLong(filter_exist());
}

static PyObject *method_destroy_filter(PyObject *self, PyObject *args)
{
    destroy_filter();
    Py_RETURN_NONE;
}


static PyMethodDef Ribbon128Methods[] =
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

static struct PyModuleDef Ribbon128Module = 
{
    PyModuleDef_HEAD_INIT,
    "ribbon128",
    "",
    -1,
    Ribbon128Methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_ribbon128(void) 
{
    //assert(!(__builtin_cpu_supports("avx2"))); //meter excepcion
    return PyModule_Create(&Ribbon128Module);
}
