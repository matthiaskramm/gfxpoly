#include <Python.h>

#include "gfxpoly.h"

// Opaque container for polygons
extern PyTypeObject PyGfxPoly;

// The canvas utility, for creating polygons
extern PyTypeObject PyGfxCanvas;

// String constants for tp_getattro. Initialized on module initialization.
static PyObject* k_gridsize;
static PyObject* k_id;

typedef struct {
    PyObject_HEAD
    gfxpoly_t* poly;
} PyGfxPolyObj;

typedef struct {
    PyObject_HEAD
    gfxcanvas_t* canvas;
} PyGfxCanvasObj;

static PyObject* WrapGfxPoly(gfxpoly_t* poly) {
    PyObject* obj = _PyObject_New(&PyGfxPoly);
    PyGfxPolyObj* poly_obj = (PyGfxPolyObj*)(obj);
    poly_obj->poly = poly;
    return obj;
}

static PyObject* WrapGfxCanvas(gfxcanvas_t* canvas) {
    PyObject* obj = _PyObject_New(&PyGfxCanvas);
    PyGfxCanvasObj* canvas_obj = (PyGfxCanvasObj*)(obj);
    canvas_obj->canvas = canvas;
    return obj;
}

static PyObject* WrapBbox(gfxbbox_t bbox) {
    PyObject* list = PyList_New(4);
    PyList_SET_ITEM(list, 0, PyFloat_FromDouble(bbox.x1));
    PyList_SET_ITEM(list, 1, PyFloat_FromDouble(bbox.y1));
    PyList_SET_ITEM(list, 2, PyFloat_FromDouble(bbox.x2));
    PyList_SET_ITEM(list, 3, PyFloat_FromDouble(bbox.y2));
    return list;
}

// --- gfxpoly ----------------------------------------------------------------

static PyObject* GfxPolyNew(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
    double gridsize = -1.0;
    static char *kwlist[] = {"gridsize", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d", kwlist, &gridsize))
        return NULL;
    gfxpoly_t* poly = gfxpoly_from_fill(NULL, gridsize);
    return WrapGfxPoly(poly);
}

static PyObject* GfxPolyArea(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxPolyObj* self = (PyGfxPolyObj*)_self;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
	return NULL;
    return PyFloat_FromDouble(gfxpoly_area(self->poly));
}

static PyObject* GfxPolyIntersect(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxPolyObj* self = (PyGfxPolyObj*)_self;
    PyGfxPolyObj* other;
    static char *kwlist[] = {"other", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyGfxPoly, &other))
	return NULL;
    return WrapGfxPoly(gfxpoly_intersect(self->poly, other->poly));
}

static PyObject* GfxPolyUnion(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxPolyObj* self = (PyGfxPolyObj*)_self;
    PyGfxPolyObj* other;
    static char *kwlist[] = {"other", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O!", kwlist, &PyGfxPoly, &other))
	return NULL;
    return WrapGfxPoly(gfxpoly_union(self->poly, other->poly));
}

static PyObject* GfxPolyMove(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxPolyObj* self = (PyGfxPolyObj*)_self;
    PyGfxPolyObj* other;
    double x, y;
    static char *kwlist[] = {"x", "y", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "dd", kwlist, &x, &y))
	return NULL;
    return WrapGfxPoly(gfxpoly_move(self->poly, x, y));
}

static PyObject* GfxPolyBBox(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxPolyObj* self = (PyGfxPolyObj*)_self;
    PyGfxPolyObj* other;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
	return NULL;
    return WrapBbox(gfxpoly_calculate_bbox(self->poly));
}

static PyObject* GfxPolyGetAttro(PyObject* _self, PyObject* attr) {
    PyGfxPolyObj* self = (PyGfxPolyObj*)_self;
    if (PyObject_RichCompareBool(attr, k_gridsize, Py_EQ) > 0) {
        return PyFloat_FromDouble(self->poly->gridsize);
    }
    return PyObject_GenericGetAttr(_self, attr);
}

static int GfxPolySetAttro(PyObject* self, PyObject* attr, PyObject* val) {
    return -1;
}

static PyObject* GfxPolyRepr(PyObject* self) {
    PyGfxPolyObj* poly_obj = (PyGfxPolyObj*)(self);
    char gridsize[80];
    snprintf(gridsize, sizeof(gridsize)-1, "%f", poly_obj->poly->gridsize);
    PyObject* str = PyUnicode_FromFormat(
            "<gfxpoly %s", gridsize);
    PyObject* final_str = PyUnicode_Concat(str, PyUnicode_FromString(">"));
    Py_DECREF(str);
    str = final_str;
    return str;
}

static void GfxPolyDealloc(PyObject* self) {
    PyGfxPolyObj* poly = (PyGfxPolyObj*)(self);
    gfxpoly_destroy(poly->poly);
    PyObject_Del(self);
}

static PyMethodDef gfxpoly_methods[] = {
    {"area", (PyCFunction)(GfxPolyArea), METH_VARARGS|METH_KEYWORDS, NULL},
    {"intersect", (PyCFunction)(GfxPolyIntersect), METH_VARARGS|METH_KEYWORDS, NULL},
    {"union", (PyCFunction)(GfxPolyUnion), METH_VARARGS|METH_KEYWORDS, NULL},
    {"bbox", (PyCFunction)(GfxPolyBBox), METH_VARARGS|METH_KEYWORDS, NULL},
    {"move", (PyCFunction)(GfxPolyMove), METH_VARARGS|METH_KEYWORDS, NULL},
    {0, 0, 0, NULL}    // sentinel
};

PyDoc_STRVAR(gfxpoly_doc, "Opaque container for polygons\n");

PyTypeObject PyGfxPoly = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    tp_name : "GfxPoly",
    tp_basicsize : sizeof(PyGfxPolyObj),
    tp_itemsize : 0,
    tp_dealloc : GfxPolyDealloc,
    tp_vectorcall_offset: -1,
    tp_getattr : NULL,
    tp_setattr : NULL,
    tp_as_async : NULL,
    tp_repr : GfxPolyRepr,
    tp_as_number : NULL,
    tp_as_sequence : NULL,
    tp_as_mapping : NULL,
    tp_hash : NULL,
    tp_call : NULL,
    tp_str : NULL,
    tp_getattro : GfxPolyGetAttro,
    tp_setattro : GfxPolySetAttro,
    tp_as_buffer : NULL,
    tp_flags : 0,
    tp_doc : gfxpoly_doc,
    tp_traverse : NULL,
    tp_clear : NULL,
    tp_richcompare : NULL,
    tp_weaklistoffset : 0,
    tp_iter : NULL,
    tp_iternext : NULL,
    tp_methods : gfxpoly_methods,
    tp_members : NULL,
    tp_getset : NULL,
    tp_base : NULL,
    tp_dict : NULL,
    tp_descr_get : NULL,
    tp_descr_set : NULL,
    tp_dictoffset : 0,
    tp_init : NULL,
    tp_alloc : NULL,
    tp_new : GfxPolyNew,
};

// --- gfxcanvas ----------------------------------------------------------------

static PyObject* GfxCanvasNew(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
    double gridsize = -1.0;
    static char *kwlist[] = {"gridsize", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "d", kwlist, &gridsize))
        return NULL;
    gfxcanvas_t* canvas = gfxcanvas_new(gridsize);
    return WrapGfxCanvas(canvas);
}

static PyObject* GfxCanvasMoveTo(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxCanvasObj* self = (PyGfxCanvasObj*)_self;
    PyGfxCanvasObj* other;
    double x, y;
    static char *kwlist[] = {"x", "y", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "dd", kwlist, &x, &y))
	return NULL;
    //self->canvas->moveTo(self->canvas, x, y);
    Py_RETURN_NONE;
}

static PyObject* GfxCanvasLineTo(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxCanvasObj* self = (PyGfxCanvasObj*)_self;
    PyGfxCanvasObj* other;
    double x, y;
    static char *kwlist[] = {"x", "y", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "dd", kwlist, &x, &y))
	return NULL;
    self->canvas->lineTo(self->canvas, x, y);
    Py_RETURN_NONE;
}

static PyObject* GfxCanvasSplineTo(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxCanvasObj* self = (PyGfxCanvasObj*)_self;
    PyGfxCanvasObj* other;
    double x, y, cx, cy;
    static char *kwlist[] = {"x", "y", "cx", "cy", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "dddd", kwlist, &x, &y, &cx, &cy))
	return NULL;
    self->canvas->splineTo(self->canvas, x, y, cx, cy);
    Py_RETURN_NONE;
}

static PyObject* GfxCanvasClose(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxCanvasObj* self = (PyGfxCanvasObj*)_self;
    PyGfxCanvasObj* other;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
	return NULL;
    self->canvas->close(self->canvas);
    Py_RETURN_NONE;
}

static PyObject* GfxCanvasResult(PyObject* _self, PyObject* args, PyObject* kwargs) {
    PyGfxCanvasObj* self = (PyGfxCanvasObj*)_self;
    PyGfxCanvasObj* other;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "", kwlist))
	return NULL;
    return WrapGfxPoly(self->canvas->result(self->canvas));
}

static PyObject* GfxCanvasGetAttro(PyObject* _self, PyObject* attr) {
    return PyObject_GenericGetAttr(_self, attr);
}

static void GfxCanvasDealloc(PyObject* self) {
    PyGfxCanvasObj* canvas = (PyGfxCanvasObj*)(self);
    // TODO: how to deallocate a canvas?
    //gfxcanvas_destroy(canvas->canvas);
    PyObject_Del(self);
}

static PyMethodDef gfxcanvas_methods[] = {
    {"moveTo", (PyCFunction)(GfxCanvasMoveTo), METH_VARARGS|METH_KEYWORDS, NULL},
    {"lineTo", (PyCFunction)(GfxCanvasLineTo), METH_VARARGS|METH_KEYWORDS, NULL},
    {"splineTo", (PyCFunction)(GfxCanvasSplineTo), METH_VARARGS|METH_KEYWORDS, NULL},
    {"close", (PyCFunction)(GfxCanvasClose), METH_VARARGS|METH_KEYWORDS, NULL},
    {"result", (PyCFunction)(GfxCanvasResult), METH_VARARGS|METH_KEYWORDS, NULL},
    {0, 0, 0, NULL}    // sentinel
};

PyDoc_STRVAR(gfxcanvas_doc, "Opaque container for canvasgons\n");

PyTypeObject PyGfxCanvas = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    tp_name : "GfxCanvas",
    tp_basicsize : sizeof(PyGfxCanvasObj),
    tp_itemsize : 0,
    tp_dealloc : GfxCanvasDealloc,
    tp_vectorcall_offset: -1,
    tp_getattr : NULL,
    tp_setattr : NULL,
    tp_as_async : NULL,
    tp_repr : NULL,
    tp_as_number : NULL,
    tp_as_sequence : NULL,
    tp_as_mapping : NULL,
    tp_hash : NULL,
    tp_call : NULL,
    tp_str : NULL,
    tp_getattro : GfxCanvasGetAttro,
    tp_setattro : NULL,
    tp_as_buffer : NULL,
    tp_flags : 0,
    tp_doc : gfxcanvas_doc,
    tp_traverse : NULL,
    tp_clear : NULL,
    tp_richcompare : NULL,
    tp_weaklistoffset : 0,
    tp_iter : NULL,
    tp_iternext : NULL,
    tp_methods : gfxcanvas_methods,
    tp_members : NULL,
    tp_getset : NULL,
    tp_base : NULL,
    tp_dict : NULL,
    tp_descr_get : NULL,
    tp_descr_set : NULL,
    tp_dictoffset : 0,
    tp_init : NULL,
    tp_alloc : NULL,
    tp_new : GfxCanvasNew,
};

// --- module -----------------------------------------------------------------

PyDoc_STRVAR(module_doc,
    "gfxpoly is a polygon intersection engine\n");

static PyMethodDef module_methods[] = {
    {0, 0, 0, NULL},    // sentinel
};

static struct PyModuleDef gfxpoly_moduledef = {
    PyModuleDef_HEAD_INIT,
    m_name : "gfxpoly",
    m_doc : module_doc,
    m_size : -1,
    m_methods : module_methods,
    m_slots : NULL,
    m_traverse : NULL,
    m_clear : NULL,
    m_free : NULL,
};

PyMODINIT_FUNC PyInit_gfxpoly(void) {
    PyObject* module = PyModule_Create(&gfxpoly_moduledef);
    PyObject* module_dict = PyModule_GetDict(module);

    PyDict_SetItemString(module_dict, "GfxPoly", (PyObject*)(&PyGfxPoly));
    PyDict_SetItemString(module_dict, "GfxCanvas", (PyObject*)(&PyGfxCanvas));

    Py_XDECREF(k_gridsize);
    k_gridsize = PyUnicode_FromString("gridsize");
    Py_XDECREF(k_id);
    k_id = PyUnicode_FromString("id");
    return module;
}
