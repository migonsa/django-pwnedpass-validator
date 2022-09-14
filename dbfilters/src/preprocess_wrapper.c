#include <Python.h>
#include <stdio.h>
#include <stdbool.h>

#include "preprocess.h"

static PyObject *method_preprocess_password_file(PyObject *self, PyObject *args, PyObject *kwargs)
{
    char* sourcefile;
    char* destfile;
    uint maxlines = -1;
    bool verify = false;

    static char *kwlist[] = {"sourcefile", "destfile", "maxlines", "verify", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|Ib", kwlist, 
                                     &sourcefile, &destfile, &maxlines, &verify)) 
        return NULL;
    
    return PyBool_FromLong(preprocess_password_file(sourcefile, destfile, maxlines, verify));
}


static PyMethodDef PreprocessMethods[] =
{
    {"preprocess_pwd_file", (PyCFunction) method_preprocess_password_file, METH_VARARGS | METH_KEYWORDS, ""},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef PreprocessModule = 
{
    PyModuleDef_HEAD_INIT,
    "preprocess",
    "",
    -1,
    PreprocessMethods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC PyInit_preprocess(void) 
{
    //assert(!(__builtin_cpu_supports("avx2"))); //meter excepcion
    return PyModule_Create(&PreprocessModule);
}