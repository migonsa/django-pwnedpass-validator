#include <Python.h>
#include <stdio.h>
#include <stdbool.h>

#include "utils.h"


static PyObject *method_password_to_hash(PyObject *self, PyObject *args)
{
    char* pass;
    char hash[40];

    if (!PyArg_ParseTuple(args, "s", &pass)) 
        return NULL;

    password2hash(pass, hash);

    return PyUnicode_FromStringAndSize(hash, 40);
}

static PyObject *method_synthetic(PyObject *self, PyObject *args)
{
    char* destfile;
    uint32_t nkeys;

    if (!PyArg_ParseTuple(args, "sI", &destfile, &nkeys)) 
        return NULL;

    return PyBool_FromLong(synthetic(destfile, nkeys));
}

static PyObject *method_calculate_keys_file(PyObject *self, PyObject *args)
{
    char* filename;

    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;
    
    return PyLong_FromUnsignedLong(calculate_nkeys(filename));
}


static PyMethodDef UtilsMethods[] =
{
    {"sha1", (PyCFunction) method_password_to_hash, METH_VARARGS, ""},
    {"synthetic", (PyCFunction) method_synthetic, METH_VARARGS, ""},
    {"calculate_keys", (PyCFunction) method_calculate_keys_file, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef UtilsModule = 
{
    PyModuleDef_HEAD_INIT,
    "utils",
    "",
    -1,
    UtilsMethods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_utils(void) 
{
    //assert(!(__builtin_cpu_supports("avx2"))); //meter excepcion
    return PyModule_Create(&UtilsModule);
}