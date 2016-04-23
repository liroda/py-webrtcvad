#include <Python.h>
#include "webrtc/common_audio/vad/include/webrtc_vad.h"

#if PY_MAJOR_VERSION >= 3
#define PY3
#endif

static PyObject *VadError;
const char webrtcvad_doc[] = "hello.";


static void vad_free(PyObject* vadptr)
{
  VadInst* handle = PyCapsule_GetPointer(vadptr, "WebRtcVadPtr");
  WebRtcVad_Free(handle);
}

static PyObject* vad_create(PyObject *self, PyObject *args)
{
  VadInst *handle;
  if (WebRtcVad_Create(&handle)) {
    return NULL;
  }
  PyObject *vadptr = PyCapsule_New(handle, "WebRtcVadPtr", vad_free);
  return Py_BuildValue("O", vadptr);
}

static PyObject* vad_init(PyObject *self, PyObject *vadptr)
{
  VadInst* handle = PyCapsule_GetPointer(vadptr, "WebRtcVadPtr");
  if (WebRtcVad_Init(handle)) {
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* vad_set_mode(PyObject *self, PyObject *args)
{
  PyObject *vadptr;
  long mode;
  if (!PyArg_ParseTuple(args, "Ol", &vadptr, &mode)) {
    return NULL;
  }
  if (mode < 0) {
    return NULL;
  } else if (mode > 3) {
    PyErr_Format(PyExc_ValueError,
                 "%d is an invalid mode, must be 0-3",
                 mode);
    return NULL;
  }
  if (WebRtcVad_set_mode(PyCapsule_GetPointer(vadptr, "WebRtcVadPtr"), mode)) {
    PyErr_Format(VadError, "Unable to set mode to %d", mode);
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* valid_rate_and_frame_length(PyObject *self, PyObject *args)
{
  long rate, frame_length;
  if (!PyArg_ParseTuple(args, "ll", &rate, &frame_length)) {
    return NULL;
  }
  if (rate > INT_MAX) {
    PyErr_Format(PyExc_ValueError,
                 "%ld is an invalid rate",
                 rate);
    return NULL;
  }
  if (frame_length > INT_MAX) {
    PyErr_Format(PyExc_ValueError,
                 "%ld is an invalid frame length",
                 frame_length);
    return NULL;
  }
  if (WebRtcVad_ValidRateAndFrameLength(rate, frame_length)) {
    Py_RETURN_FALSE;
  } else {
    Py_RETURN_TRUE;
  }
}

static PyObject* vad_process(PyObject *self, PyObject *args)
{
  PyObject *vadptr;
  long fs;
  Py_buffer audio_frame = {NULL, NULL};
  long frame_length;
  if (!PyArg_ParseTuple(args, "Oly*l", &vadptr, &fs, &audio_frame, &frame_length)) {
    return NULL;
  }
  int result =  WebRtcVad_Process(PyCapsule_GetPointer(vadptr, "WebRtcVadPtr"),
                                  fs,
                                  audio_frame.buf,
                                  frame_length);
  switch (result) {
  case 1:
    Py_RETURN_TRUE;
    break;
  case 0:
    Py_RETURN_FALSE;
    break;
  case -1:
    PyErr_Format(VadError, "Error while processing frame");
  }
  return NULL;
}

static PyMethodDef VadMethods[] = {
    {"create",  vad_create, METH_NOARGS,
     "Create a vad."},
    {"init",  vad_init, METH_O,
     "Init a vad."},
    {"set_mode",  vad_set_mode, METH_VARARGS,
     "Set mode."},
    {"process",  vad_process, METH_VARARGS,
     "Set mode."},
    {"valid_rate_and_frame_length", valid_rate_and_frame_length, METH_VARARGS,
     "Set mode."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef webrtcvad_module = {
   PyModuleDef_HEAD_INIT,
   "_webrtcvad",   /* name of module */
   webrtcvad_doc, /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   VadMethods
};

PyMODINIT_FUNC
PyInit__webrtcvad(void)
{
    PyObject *m;

    m = PyModule_Create(&webrtcvad_module);
    if (m == NULL)
        return NULL;

    VadError = PyErr_NewException("webrtcvad.Error", NULL, NULL);
    Py_INCREF(VadError);
    PyModule_AddObject(m, "Error", VadError);
    return m;
}
