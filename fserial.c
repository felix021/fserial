#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <Python.h>
#include <assert.h>
#include <longintrepr.h>

static PyObject * fserial_dumps(PyObject *self, PyObject *args);
static PyObject * fserial_loads(PyObject *self, PyObject *args);
static PyObject * fserial_setbufsize(PyObject *self, PyObject *args);

static PyObject *fserial_error; 
PyMODINIT_FUNC initfserial(void);

#define TP_String   's'
#define TP_Int64    'i'
#define TP_Long     'l'
#define TP_Float    'd'
#define TP_None     'N'
#define TP_True     't'
#define TP_False    'f'
#define TP_Dict     'D'
#define TP_List     'L'
#define TP_Tuple    'T'
#define TP_Set      'S'

static PyMethodDef fserial_methods[] = {
    {"dumps", fserial_dumps, METH_VARARGS, ""},
    {"loads", fserial_loads, METH_VARARGS, ""},
    {"setbufsize", fserial_setbufsize, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}
};

static int bufsize = 65536;
static int use_stack_limit = 65536;

PyMODINIT_FUNC initfserial(void)
{
    PyObject *m = Py_InitModule("fserial", fserial_methods);
    if (m == NULL)
        return;

    fserial_error = PyErr_NewException("fserial.error", NULL, NULL);
    Py_INCREF(fserial_error);
    PyModule_AddObject(m, "error", fserial_error);
}

static PyObject * fserial_setbufsize(PyObject *self, PyObject *args)
{
    int new_bufsize;
    if (!PyArg_ParseTuple(args, "i:loads", &new_bufsize))
        return NULL;
    bufsize = new_bufsize;
    Py_INCREF(Py_None);
    return Py_None;
}

static int w_object(char *result, PyObject *value)
{
    int idx = 0;
    if (value == Py_None) {
        *result = TP_None;
        idx += 1;
    }
    else if (value == Py_True) {
        *result = TP_True;
        idx += 1;
    }
    else if (value == Py_False) {
        *result = TP_False;
        idx += 1;
    }
    else if (PyInt_CheckExact(value)) {
        *result = TP_Int64;
        *(int64_t *)(result + 1) = PyInt_AsLong(value);
        idx += 1 + sizeof(int64_t);
    }
    else if (PyLong_CheckExact(value)) {
        //ndigit can be negtive, indicates that this long is negtive
        int32_t ndigit = Py_SIZE(value), size = abs(ndigit) * sizeof(((PyLongObject *)value)->ob_digit[0]);
        *result = TP_Long;
        *(int32_t *)(result + 1) = ndigit;
        memcpy(result + 1 + sizeof(int32_t), ((PyLongObject *)value)->ob_digit, size);
        idx += 1 + sizeof(size) + size;
    }
    else if (PyFloat_CheckExact(value)) {
        *result = TP_Float;
        *(double *)(result + 1) = PyFloat_AsDouble(value);
        idx += 1 + sizeof(double);
    }
    else if (PyString_CheckExact(value)) {
        int size = Py_SIZE(value);
        *result = TP_String;
        *(int32_t *)(result + 1) = size;
        memcpy(result + 1 + sizeof(int32_t), ((PyStringObject *)value)->ob_sval, size);
        idx += 1 + sizeof(int32_t) + size;
    }
    else if (PyList_CheckExact(value)) {
        *result = TP_List;
        *(int32_t *)(result + 1) = PyList_GET_SIZE(value);
        idx += 1 + sizeof(int32_t);
        int i = 0;
        for (i = 0; i < Py_SIZE(value); i++)
            idx += w_object(result + idx, PyList_GET_ITEM(value, i));
    }
    else if (PyTuple_CheckExact(value)) {
        *result = TP_Tuple;
        *(int32_t *)(result + 1) = PyTuple_Size(value);
        idx += 1 + sizeof(int32_t);
        int i = 0;
        for (i = 0; i < Py_SIZE(value); i++)
            idx += w_object(result + idx, PyTuple_GET_ITEM(value, i));
    }
    else if (PyDict_CheckExact(value)) {
        *result = TP_Dict;
        *(int32_t *)(result + 1) = Py_SIZE(value);
        idx += 1 + sizeof(int32_t);

        Py_ssize_t dict_idx = 0;
        PyObject *key, *subvalue;
        while (PyDict_Next(value, &dict_idx, &key, &subvalue)) {
            idx += w_object(result + idx, key);
            idx += w_object(result + idx, subvalue);
        }
    }
    else if (PyAnySet_CheckExact(value)) {
        PyObject *subvalue, *it;
        *result = TP_Set;
        *(int32_t *)(result + 1) = Py_SIZE(value);
        idx += 1 + sizeof(int32_t);

        it = PyObject_GetIter(value);
        while ((subvalue = PyIter_Next(it)) != NULL) {
            idx += w_object(result + idx, subvalue);
            Py_DECREF(subvalue);
        }
        Py_DECREF(it);
    }
    else {
        PyErr_Format(fserial_error, "dumps: unknown value type!");
        return -bufsize;
    }
    return idx >= 0 ? idx : -bufsize;
}

static PyObject * fserial_dumps(PyObject *self, PyObject *args)
{
    int32_t idx = 0; 
    PyObject *x;
    if (!PyArg_ParseTuple(args, "O:dumps", &x))
        return NULL;

    char *result_buf = NULL;
    if (bufsize <= use_stack_limit)
        result_buf = (char *)alloca(bufsize);
    else
        result_buf = (char *)malloc(bufsize);
    assert(result_buf != 0);

    idx = w_object(result_buf, x);
    if (idx < 0) {
        if (bufsize > use_stack_limit)
            free(result_buf);
        return NULL;
    }

    PyObject *result = PyString_FromStringAndSize(result_buf, idx);
    if (bufsize > use_stack_limit)
        free(result_buf);
    return result;
}

static PyObject *r_object(char *s, int *idx)
{
    PyObject *result;
    int32_t i, val_size, ndigit;
    int64_t ival;
    double fval;
    PyLongObject *lresult;

    switch(s[0])
    {
        case TP_String:
            val_size = *(int32_t *)(s + 1);
            *idx = 1 + sizeof(int32_t) + val_size;
            return (PyObject *)PyString_FromStringAndSize(s + 1 + sizeof(int32_t), val_size);
        case TP_Int64:
            ival = *(int64_t *)(s + 1);
            *idx = 1 + sizeof(int64_t);
            return PyInt_FromLong(ival);
        case TP_Long:
            ndigit   = *(int32_t *)(s + 1);
            lresult  = _PyLong_New(abs(ndigit));
            val_size = abs(ndigit) * sizeof(lresult->ob_digit[0]);
            fprintf(stderr, "long ndigit: %d, size:%d, lresult = %p\n", ndigit, val_size, lresult);
            if (lresult != NULL) {
                lresult->ob_size = ndigit;
                memcpy(lresult->ob_digit, s + 1 + sizeof(ndigit), val_size);
                *idx = 1 + sizeof(int32_t) + val_size;
                for (i = 0; i < val_size; i++) {
                    printf("%02u ", *(s + 1 + sizeof(ndigit) + i));
                }
                printf("\n");
            }
            return (PyObject *)lresult;
        case TP_Float:
            fval = *(double *)(s + 1);
            *idx = 1 + sizeof(double);
            return PyFloat_FromDouble(fval);
        case TP_None:
            *idx = 1;
            Py_INCREF(Py_None);
            return Py_None;
        case TP_True:
            *idx = 1;
            Py_INCREF(Py_True);
            return Py_True;
        case TP_False:
            *idx = 1;
            Py_INCREF(Py_False);
            return Py_False;
        case TP_List:
            *idx = 1 + sizeof(val_size);
            val_size = *(int32_t *)(s + 1);
            result = PyList_New(val_size);
            for (i = 0; i < val_size; i++)
            {
                int subidx;
                PyObject *subvalue = r_object(s + *idx, &subidx);
                if (subvalue == NULL)//TODO
                    return NULL;
                *idx += subidx;
                PyList_SET_ITEM(result, i, subvalue);
            }
            return result;
        case TP_Tuple:
            *idx = 1 + sizeof(val_size);
            val_size = *(int32_t *)(s + 1);
            result = PyTuple_New(val_size);
            for (i = 0; i < val_size; i++)
            {
                int subidx;
                PyObject *subvalue = r_object(s + *idx, &subidx);
                if (subvalue == NULL)//TODO
                    return NULL;
                *idx += subidx;
                PyTuple_SET_ITEM(result, i, subvalue);
            }
            return result;
        case TP_Set:
            *idx = 1 + sizeof(val_size);
            val_size = *(int32_t *)(s + 1);
            result = PySet_New(NULL);
            for (i = 0; i < val_size; i++)
            {
                int subidx;
                PyObject *subvalue = r_object(s + *idx, &subidx);
                if (subvalue == NULL) //TODO
                    return NULL;
                *idx += subidx;
                PySet_Add(result, subvalue);
                Py_DECREF(subvalue); //PySet_Add will do INCREF on subvalue
            }
            return result;
        case TP_Dict:
            *idx = 1 + sizeof(val_size);
            val_size = *(int32_t *)(s + 1);
            result = PyDict_New();
            if (result == NULL) {
                PyErr_Format(fserial_error, "PyDict_New failed");
                return NULL;
            }
            for (i = 0; i < (int)val_size; i++)
            {
                int subidx;
                PyObject *key, *subvalue;
                key = r_object(s + *idx, &subidx);
                *idx += subidx;
                subvalue = r_object(s + *idx, &subidx);
                *idx += subidx;
                PyDict_SetItem(result, key, subvalue);
                Py_DECREF(key);
                Py_DECREF(subvalue);
            }
            return result;
        default:
            PyErr_Format(fserial_error, "key is not string! unkown type!");
            return NULL;
    }
}

static PyObject * fserial_loads(PyObject *self, PyObject *args)
{
    char *s;
    int idx, n;
    PyObject* result;
    if (!PyArg_ParseTuple(args, "s#:loads", &s, &n))
        return NULL;

    result = r_object(s, &idx);
    if (result == NULL) {
        PyErr_Format(fserial_error, "loads failed");
        return NULL;
    }

    if (idx != n) {
        PyErr_Format(fserial_error, "loads imcomplete: %d of %d bytes is used", idx, n);
    }

    return result;
}

