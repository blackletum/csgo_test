﻿/* Complex number structure */

#ifndef Py_COMPLEXOBJECT_H
#define Py_COMPLEXOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double real;
    double imag;
} Py_complex;

/* Operations on complex numbers from complexmodule.c */

#define c_sum _Py_c_sum
#define c_diff _Py_c_diff
#define c_neg _Py_c_neg
#define c_prod _Py_c_prod
#define c_quot _Py_c_quot
#define c_pow _Py_c_pow

PyAPI_FUNC(Py_complex) c_sum(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) c_diff(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) c_neg(Py_complex);
PyAPI_FUNC(Py_complex) c_prod(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) c_quot(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) c_pow(Py_complex, Py_complex);


/* Complex object interface */

/*
PyComplexObject represents a complex number with double-precision
real and imaginary parts.
*/

typedef struct {
    PyObject_HEAD
    Py_complex cval;
} PyComplexObject;     

PyAPI_DATA(PyTypeObject) PyComplex_Type;

#define PyComplex_Check(op) PyObject_TypeCheck(op, &PyComplex_Type)
#define PyComplex_CheckExact(op) ((op)->ob_type == &PyComplex_Type)

PyAPI_FUNC(PyObject *) PyComplex_FromCComplex(Py_complex);
PyAPI_FUNC(PyObject *) PyComplex_FromDoubles(double real, double imag);

PyAPI_FUNC(double) PyComplex_RealAsDouble(PyObject *op);
PyAPI_FUNC(double) PyComplex_ImagAsDouble(PyObject *op);
PyAPI_FUNC(Py_complex) PyComplex_AsCComplex(PyObject *op);

#ifdef __cplusplus
}
#endif
#endif /* !Py_COMPLEXOBJECT_H */
