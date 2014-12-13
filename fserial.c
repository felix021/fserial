#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <Python.h>

#define WITH_BOOL   1

static PyObject * fserial_dumps(PyObject *self, PyObject *args);
static PyObject * fserial_loads(PyObject *self, PyObject *args);

static PyObject *fserial_error; 
PyMODINIT_FUNC initfserial(void);

#define TP_String   's'
#define TP_Int64    'i'
#define TP_Float    'f'
#define TP_None     'N'
#define TP_True     'T'
#define TP_False    'F'

static PyMethodDef fserial_methods[] = {
    {"dumps", fserial_dumps, METH_VARARGS, ""},
    {"loads", fserial_loads, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initfserial(void)
{
    PyObject *m = Py_InitModule("fserial", fserial_methods);
    if (m == NULL)
        return;

    fserial_error = PyErr_NewException("fserial.error", NULL, NULL);
    Py_INCREF(fserial_error);
    PyModule_AddObject(m, "error", fserial_error);
}

static int write_string(char *buf, char *val, u_int32_t size)
{
    *buf = TP_String;
    *(u_int32_t *)(buf + 1) = size;
    memcpy(buf + 1 + sizeof(u_int32_t), val, size);
    return 1 + sizeof(u_int32_t) + size;
}

static int write_int64(char *buf, int64_t val)
{
    *buf = TP_Int64;
    *(int64_t *)(buf + 1) = val;
    return 1 + sizeof(val);
}

static int write_float(char *buf, double val)
{
    *buf = TP_Float;
    *(double *)(buf + 1) = val;
    return 1 + sizeof(val);
}

static int write_none(char *buf)
{
    *buf = TP_None;
    return 1;
}

static int write_true(char *buf)
{
    *buf = TP_True;
    return 1;
}

static int write_false(char *buf)
{
    *buf = TP_False;
    return 1;
}

static PyObject * fserial_dumps(PyObject *self, PyObject *args)
{
    char result[65536]; //max: 64k
    u_int32_t idx = 0; 
    PyObject *x;
    if (!PyArg_ParseTuple(args, "O:dumps", &x))
        return NULL;
    Py_ssize_t dict_idx = 0;
    PyObject *key, *value;
    while (PyDict_Next(x, &dict_idx, &key, &value)) {
        //Refer to borrowed references in key and value.
        if (!PyString_CheckExact(key)) {
            PyErr_Format(fserial_error, "key is not str");
            return NULL;
        }
        idx += write_string(result + idx, ((PyStringObject*)key)->ob_sval, Py_SIZE(key));

        if (value == Py_None) {
            idx += write_none(result + idx);
        }
#if WITH_BOOL == 1
        else if (value == Py_True) {
            idx += write_true(result + idx);
        }
        else if (value == Py_False) {
            idx += write_false(result + idx);
        }
#endif
        else if (PyInt_CheckExact(value)) {
            idx += write_int64(result + idx, PyInt_AsLong(value));
        }
        else if (PyLong_CheckExact(value)) {
            int64_t v;
            int overflow;
            v = PyLong_AsLongAndOverflow(value, &overflow);
            if (overflow != 0) {
                PyErr_Format(fserial_error, "PyLong_AsLongAndOverflow overflow");
                return NULL;
            }
            idx += write_int64(result + idx, v);
        }
        else if (PyFloat_CheckExact(value)) {
            idx += write_float(result + idx, PyFloat_AsDouble(value));
        }
        else if (PyString_CheckExact(value)) {
            idx += write_string(result + idx, ((PyStringObject*)value)->ob_sval, Py_SIZE(value));
        }
        else {
            PyErr_Format(fserial_error, "unknown value type!");
            return NULL;
        }
    }
    return PyString_FromStringAndSize(result, idx);
}

static int read_string(char *buf, char **val, u_int32_t *size)
{
    *size = *(u_int32_t *)(buf + 1);
    *val  = buf + 1 + sizeof(u_int32_t);
    return 1 + sizeof(u_int32_t) + *size;
}

static PyObject * fserial_loads(PyObject *self, PyObject *args)
{
    char *s;
    u_int32_t idx = 0, n;
    PyObject* result;
    if (!PyArg_ParseTuple(args, "s#:loads", &s, &n))
        return NULL;

    result = PyDict_New();
    if (result == NULL) {
        PyErr_Format(fserial_error, "PyDict_New failed");
        return NULL;
    }

    char *skey, *sval;
    u_int32_t key_size, val_size;
    PyObject *okey, *oval;
    int64_t ival;
    double fval;

    while (idx < n)
    {
        //key
        if (s[idx] != TP_String) {
            PyErr_Format(fserial_error, "key is not string! unkown type!");
            return NULL;
        }
        idx += read_string(s + idx, &skey, &key_size);
        okey = PyString_FromStringAndSize(skey, key_size);

        //value
        switch(s[idx])
        {
            case TP_String:
                idx += read_string(s + idx, &sval, &val_size);
                oval = (PyObject *)PyString_FromStringAndSize(sval, val_size);
                break;
            case TP_Int64:
                ival = *(int64_t *)(s + idx + 1);
                idx += 1 + sizeof(int64_t);
                oval = PyInt_FromLong(ival);
                break;
            case TP_Float:
                fval = *(double *)(s + idx + 1);
                idx += 1 + sizeof(double);
                oval = PyFloat_FromDouble(fval);
                break;
            case TP_None:
                oval = Py_None;
                idx += 1;
                Py_INCREF(oval);
                break;
#if WITH_BOOL == 1
            case TP_True:
                oval = Py_True;
                idx += 1;
                Py_INCREF(oval);
                break;
            case TP_False:
                oval = Py_False;
                idx += 1;
                Py_INCREF(oval);
                break;
#endif
            default:
                PyErr_Format(fserial_error, "key is not string! unkown type!");
                return NULL;
        }

        PyDict_SetItem(result, okey, oval);
        Py_DECREF(okey);
        Py_DECREF(oval);
    }
    return result;
}

