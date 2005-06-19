
/*
  Meanwhile - Unofficial Lotus Sametime Community Client Library
  Copyright (C) 2004  Christopher (siege) O'Brien
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.
  
  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <Python.h>
#include <structmember.h>

#include <glib.h>
#include <glib/ghash.h>
#include <glib/glist.h>
#include <string.h>

#include <mw_common.h>
#include <mw_service.h>
#include <mw_srvc_ft.h>

#include "py_meanwhile.h"
#include "mw_error.h"
#include "mw_debug.h"


#define ON_OFFERED  "onTransferOffered"
#define ON_OPENED   "onTrasferOpened"
#define ON_CLOSED   "onTransferClosed"
#define ON_RECV     "onTransferRecv"
#define ON_ACK      "onTransferAck"


struct ft_data {
  GHashTable *map;
  guint32 counter;
};


static struct ft_data *ft_data_new() {
  struct ft_data *data = g_new0(struct ft_data, 1);
  data->map = g_hash_table_new(g_direct_hash, g_direct_equal);
  data->counter++;
  return data;
}


static void ft_data_free(gpointer d) {
  struct ft_data *data = d;
  g_hash_table_destroy(data->map);
  g_free(data);
}


static void ft_remove(mwPyService *self, guint32 id) {
  struct ft_data *ftd = self->data;
  g_hash_table_remove(ftd->map, GUINT_TO_POINTER(id));
}


static void ft_put(mwPyService *self, struct mwFileTransfer *ft) {
  struct ft_data *ftd = self->data;
  guint32 id = ftd->counter++;
  g_hash_table_insert(ftd->map, GUINT_TO_POINTER(id), ft);
  mwFileTransfer_setClientData(ft, GUINT_TO_POINTER(id), NULL);
}


static struct mwFileTransfer *ft_get(mwPyService *self, guint32 id) {
  struct ft_data *ftd = self->data;
  return g_hash_table_lookup(ftd->map, GUINT_TO_POINTER(id));
}


static void mw_ft_offered(struct mwFileTransfer *ft) {
  struct mwServiceFileTransfer *srvc;
  mwPyService *self;
  PyObject *robj = NULL;
  guint32 id;

  id = GPOINTER_TO_UINT(mwFileTransfer_getClientData(ft));

  srvc = mwFileTransfer_getService(ft);
  self = mwService_getClientData(MW_SERVICE(srvc));

  robj = PyObject_CallMethod((PyObject *) self, ON_OFFERED, "l", id);
  Py_XDECREF(robj);
}


static void mw_ft_opened(struct mwFileTransfer *ft) {
  struct mwServiceFileTransfer *srvc;
  mwPyService *self;
  PyObject *robj = NULL;
  guint32 id;

  id = GPOINTER_TO_UINT(mwFileTransfer_getClientData(ft));

  srvc = mwFileTransfer_getService(ft);
  self = mwService_getClientData(MW_SERVICE(srvc));

  robj = PyObject_CallMethod((PyObject *) self, ON_OPENED, "l", id);
  Py_XDECREF(robj);
}


static void mw_ft_closed(struct mwFileTransfer *ft, guint32 code) {
  struct mwServiceFileTransfer *srvc;
  mwPyService *self;
  PyObject *robj = NULL;
  guint32 id;

  id = GPOINTER_TO_UINT(mwFileTransfer_getClientData(ft));

  srvc = mwFileTransfer_getService(ft);
  self = mwService_getClientData(MW_SERVICE(srvc));

  robj = PyObject_CallMethod((PyObject *) self, ON_CLOSED, "ll", id, code);
  Py_XDECREF(robj);

  ft_remove(self, id);
  mwFileTransfer_free(ft);
}


static void mw_ft_recv(struct mwFileTransfer *ft, struct mwOpaque *data) {
  struct mwServiceFileTransfer *srvc;
  mwPyService *self;
  PyObject *robj, *pbuf;
  guint32 id;

  id = GPOINTER_TO_UINT(mwFileTransfer_getClientData(ft));

  srvc = mwFileTransfer_getService(ft);
  self = mwService_getClientData(MW_SERVICE(srvc));

  pbuf = PyBuffer_FromMemory(data->data, data->len);

  robj = PyObject_CallMethod((PyObject *) self, ON_RECV, "lO", id, pbuf);
  Py_XDECREF(robj);
}


static void mw_ft_ack(struct mwFileTransfer *ft) {
  struct mwServiceFileTransfer *srvc;
  mwPyService *self;
  PyObject *robj = NULL;
  guint32 id;

  id = GPOINTER_TO_UINT(mwFileTransfer_getClientData(ft));

  srvc = mwFileTransfer_getService(ft);
  self = mwService_getClientData(MW_SERVICE(srvc));

  robj = PyObject_CallMethod((PyObject *) self, ON_ACK, "l", id);
  Py_XDECREF(robj);
}


static PyObject *py_offer(mwPyService *self, PyObject *args) {
  struct mwServiceFileTransfer *srvc;
  guint32 id;
  
  PyObject *a, *b, *c, *d;
  struct mwIdBlock idb = { 0, 0 };
  const char *msg, *filename;
  guint32 size;
  struct mwFileTransfer *ft;
  
  srvc = (struct mwServiceFileTransfer *) self->wrapped;
  
  if(! PyArg_ParseTuple(args, "(OO)OOl", &a, &b, &c, &d, &size))
    return NULL;
  
  idb.user = (char *) PyString_SafeAsString(a);
  idb.community = (char *) PyString_SafeAsString(b);
  msg = PyString_SafeAsString(c);
  filename = PyString_SafeAsString(d);
  
  ft = mwFileTransfer_new(srvc, &idb, msg, filename, size);
  
  if(mwFileTransfer_offer(ft)) {
    mwFileTransfer_free(ft);
    mw_raise("error offering file transfer", NULL);
  }
  
  ft_put(self, ft);
  id = GPOINTER_TO_UINT(mwFileTransfer_getClientData(ft));
  return PyInt_FromLong(id);
}


static PyObject *py_accept(mwPyService *self, PyObject *args) {
  struct mwFileTransfer *ft;
  guint32 id;
  
  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  ft = ft_get(self, id);
  if(! ft) mw_raise("no such file transfer", NULL);

  if(mwFileTransfer_accept(ft))
    mw_raise("error accepting file transfer", NULL);

  mw_return_none();
}


static PyObject *py_close(mwPyService *self, PyObject *args) {
  struct mwFileTransfer *ft;
  guint32 id = 0, code = mwFileTransfer_SUCCESS;

  if(! PyArg_ParseTuple(args, "l|l", &id, &code))
    return NULL;

  ft = ft_get(self, id);
  if(! ft) mw_raise("no such file transfer", NULL);

  if(mwFileTransfer_close(ft, code))
    mw_raise("error closing file transfer", NULL);

  mw_return_none();
}


static PyObject *py_send(mwPyService *self, PyObject *args) {
  struct mwFileTransfer *ft;
  guint32 id = 0;
  struct mwOpaque o = { 0, 0 };

  if(! PyArg_ParseTuple(args, "lt#", &id, &o.data, &o.len))
    return NULL;

  ft = ft_get(self, id);
  if(! ft) mw_raise("no such file transfer", NULL);

  if(mwFileTransfer_send(ft, &o))
    mw_raise("error sending file transfer data", NULL);

  mw_return_none();
}


static PyObject *py_ack(mwPyService *self, PyObject *args) {
  struct mwFileTransfer *ft;
  guint32 id = 0;

  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  ft = ft_get(self, id);
  if(! ft) mw_raise("no such file transfer", NULL);

  if(mwFileTransfer_ack(ft))
    mw_raise("error sending file transfer ACK", NULL);

  mw_return_none();
}


static PyObject *py_list(mwPyService *self) {
  struct mwServiceFileTransfer *srvc;
  const GList *transfers;
  PyObject *t;
  int i;

  srvc = (struct mwServiceFileTransfer *) self->wrapped;
  transfers = mwServiceFileTransfer_getTransfers(srvc);

  t = PyTuple_New(g_list_length(transfers));
  for(i = 0; transfers; transfers = transfers->next) {
    struct mwFileTransfer *ft;
    guint32 id;

    ft = transfers->data;
    id = GPOINTER_TO_INT(mwFileTransfer_getClientData(ft));
    PyTuple_SetItem(t, i++, PyInt_FromLong(id));
  }

  return t;
}


static struct PyMethodDef tp_methods[] = {
  { ON_OFFERED, MW_METH_VARARGS_NONE, METH_VARARGS,
    "override to receive ..." },

  { ON_OPENED, MW_METH_VARARGS_NONE, METH_VARARGS,
    "override to receive ..." },

  { ON_CLOSED, MW_METH_VARARGS_NONE, METH_VARARGS,
    "override to receive ..." },

  { ON_RECV, MW_METH_VARARGS_NONE, METH_VARARGS,
    "override to receive ..." },

  { ON_ACK, MW_METH_VARARGS_NONE, METH_VARARGS,
    "override to receive ..." },

  { "transferOffer", (PyCFunction) py_offer, METH_VARARGS,
    "" },

  { "transferAccept", (PyCFunction) py_accept, METH_VARARGS,
    "" },

  { "transferClose", (PyCFunction) py_close, METH_VARARGS,
    "" },

  { "transferSend", (PyCFunction) py_send, METH_VARARGS,
    "" },

  { "transferAck", (PyCFunction) py_ack, METH_VARARGS,
    "" },

  { "transfers", (PyCFunction) py_list, METH_NOARGS,
    "" },

  { NULL }
};


static PyGetSetDef tp_getset[] = {
  { NULL }
};


static struct mwFileTransferHandler handler = {
  .ft_offered = mw_ft_offered,
  .ft_opened = mw_ft_opened,
  .ft_closed = mw_ft_closed,
  .ft_recv = mw_ft_recv,
  .ft_ack = mw_ft_ack,
  .clear = NULL,
};


static PyObject *tp_new(PyTypeObject *t, PyObject *args, PyObject *kwds) {
  mwPyService *self;
  mwPySession *sessobj;
  struct mwSession *session;
  struct mwServiceFileTransfer *srvc_ft;
  
  if(! PyArg_ParseTuple(args, "O", &sessobj))
    return NULL;

  if(! PyObject_IsInstance((PyObject *) sessobj,
			   (PyObject *) mwPySession_type()))
    return NULL;

  self = (mwPyService *) t->tp_alloc(t, 0);

  /* wrapped version of the underlying service's session */
  Py_INCREF(sessobj);
  self->session = sessobj;
  session = sessobj->session;

  /* create the im service */
  srvc_ft = mwServiceFileTransfer_new(session, &handler);

  /* store the self object on the service's client data slot */
  mwService_setClientData(MW_SERVICE(srvc_ft), self, NULL);

  /* create a python wrapper service built around this instance */
  /* sets self->wrapper and self->service */
  mwServicePyWrap_wrap(self, MW_SERVICE(srvc_ft));

  self->data = ft_data_new();
  self->cleanup = ft_data_free;
  
  return (PyObject *) self;
}


static PyTypeObject pyType_mwServiceFileTransfer = {
  PyObject_HEAD_INIT(NULL)
  0, /* ob_size */
  "_meanwhile.ServiceFileTransfer",
  sizeof(mwPyService),
  0, /* tp_itemsize */
  0, /* tp_dealloc */
  0, /* tp_print */
  0, /* tp_getattr */
  0, /* tp_setattr */
  0, /* tp_compare */
  0, /* tp_repr */
  0, /* tp_as_number */
  0, /* tp_as_sequence */
  0, /* tp_as_mapping */
  0, /* tp_hash */
  0, /* tp_call */
  0, /* tp_str */
  0, /* tp_getattro */
  0, /* tp_setattro */
  0, /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
  "Meanwhile client service for file transfer",
  0, /* tp_traverse */
  0, /* tp_clear */
  0, /* tp_richcompare */
  0, /* tp_weaklistoffset */
  0, /* tp_iter */
  0, /* tp_iternext */
  tp_methods,
  0, /* tp_members */
  tp_getset,
  0, /* tp_base */
  0, /* tp_dict */
  0, /* tp_descr_get */
  0, /* tp_descr_set */
  0, /* tp_dictoffset */
  0, /* tp_init */
  0, /* tp_alloc */
  (newfunc) tp_new,
};


static PyTypeObject *py_type = NULL;


PyTypeObject *mwPyServiceFileTransfer_type() {
  if(! py_type) {
    g_message("readying type mwPyServiceFileTransfer");

    py_type = &pyType_mwServiceFileTransfer;
    py_type->tp_base = mwPyService_type();

    Py_INCREF(py_type);
    PyType_Ready(py_type);
  }

  return py_type;
}

