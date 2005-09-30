
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

#include <glib/glist.h>

#include <mw_channel.h>
#include <mw_cipher.h>
#include <mw_service.h>
#include <mw_session.h>

#include "py_meanwhile.h"
#include "mw_util.h"


#define ON_IO_WRITE      "onIoWrite"
#define ON_IO_CLOSE      "onIoClose"
#define ON_STATE_CHANGE  "onStateChange"
#define ON_SET_PRIVACY   "onSetPrivacy"
#define ON_SET_USER      "onSetUserStatus"
#define ON_ADMIN         "onAdmin"
#define ON_ANNOUNCE      "onAnnounce"


static int mw_io_write(struct mwSession *s, const char *buf, gsize len) {
  PyObject *sobj, *robj;
  PyObject *pbuf;
  int ret = -1;
  
  sobj = mwSession_getClientData(s);
  g_return_val_if_fail(sobj != NULL, -1);

  pbuf = PyBuffer_FromMemory((char *) buf, len);
  g_return_val_if_fail(pbuf != NULL, -1);

  robj = PyObject_CallMethod(sobj, ON_IO_WRITE, "N", pbuf);

  if(robj && PyInt_Check(robj))
    ret = (int) PyInt_AsLong(robj);

  Py_XDECREF(robj);

  return ret;
}


static void mw_io_close(struct mwSession *s) {
  PyObject *sobj, *robj;

  sobj = mwSession_getClientData(s);
  g_return_if_fail(sobj != NULL);

  robj = PyObject_CallMethod(sobj, ON_IO_CLOSE, NULL);
  Py_XDECREF(robj);
}


static void mw_clear(struct mwSession *s) {
  g_free(mwSession_getHandler(s));
}


static void mw_on_state(struct mwSession *s,
			enum mwSessionState state, gpointer info) {

  PyObject *sobj, *robj;

  sobj = mwSession_getClientData(s);
  g_return_if_fail(sobj != NULL);

  robj = PyObject_CallMethod(sobj, ON_STATE_CHANGE, NULL);
  Py_XDECREF(robj);
}


static void mw_on_privacy(struct mwSession *s) {
  PyObject *sobj, *robj;

  sobj = mwSession_getClientData(s);
  g_return_if_fail(sobj != NULL);

  robj = PyObject_CallMethod(sobj, ON_SET_PRIVACY, NULL);
  Py_XDECREF(robj);
}


static void mw_on_user(struct mwSession *s) {
  PyObject *sobj, *robj;

  sobj = mwSession_getClientData(s);
  g_return_if_fail(sobj != NULL);

  robj = PyObject_CallMethod(sobj, ON_SET_USER, NULL);
  Py_XDECREF(robj);
}


static void mw_on_admin(struct mwSession *s, const char *text) {
  PyObject *sobj, *robj;
  PyObject *a;

  sobj = mwSession_getClientData(s);
  g_return_if_fail(sobj != NULL);

  a = PyString_SafeFromString(text);

  robj = PyObject_CallMethod(sobj, ON_ADMIN, "N", a);
  Py_XDECREF(robj);
}


static void mw_on_announce(struct mwSession *s, struct mwLoginInfo *from,
			   gboolean may_reply, const char *text) {
  PyObject *sobj, *robj;
  PyObject *b, *c;

  sobj = mwSession_getClientData(s);
  g_return_if_fail(sobj != NULL);

  b = PyInt_FromLong(may_reply);
  c = PyString_SafeFromString(text);

  robj = PyObject_CallMethod(sobj, ON_ANNOUNCE, "NN", b, c);
  Py_XDECREF(robj);
}


static PyObject *py_io_write(mwPySession *self, PyObject *args) {
  return PyInt_FromLong(-1);
}


static PyObject *py_start(mwPySession *self, PyObject *args) {
  PyObject *uo, *po;
  char *user, *pass;

  if(! PyArg_ParseTuple(args, "(OO)", &uo, &po))
    return NULL;

  user = (char *) PyString_SafeAsString(uo);
  pass = (char *) PyString_SafeAsString(po);

  mwSession_setProperty(self->session, mwSession_AUTH_USER_ID, user, NULL);
  mwSession_setProperty(self->session, mwSession_AUTH_PASSWORD, pass, NULL);

  mwSession_start(self->session);
  mw_return_none();
}


static PyObject *py_stop(mwPySession *self, PyObject *args) {
  guint32 reason = 0x00;
  
  if(! PyArg_ParseTuple(args, "|l", &reason))
    return NULL;

  mwSession_stop(self->session, reason);
  mw_return_none();
}


static PyObject *py_recv(mwPySession *self, PyObject *args) {
  char *buf;
  gsize len;

  if(! PyArg_ParseTuple(args, "t#", &buf, &len))
    return NULL;

  mwSession_recv(self->session, buf, len);
  mw_return_none();
}


static PyObject *py_chan_create(mwPySession *self, PyObject *args) {
  struct mwSession *s;
  struct mwChannel *c;
  
  mwPyService *srvcobj;
  guint32 proto_type, proto_ver;
  struct mwOpaque o = { 0, 0 };
  
  if(! PyArg_ParseTuple(args, "Oll|t#",
			&srvcobj, &proto_type, &proto_ver,
			&o.data, &o.len)) {
    return NULL;
  }

  s = self->session;
  c = mwChannel_newOutgoing(mwSession_getChannels(s));

  mwChannel_setService(c, srvcobj->wrapped);
  mwChannel_setProtoType(c, proto_type);
  mwChannel_setProtoVer(c, proto_ver);

  if(o.len) {
    struct mwOpaque *co = mwChannel_getAddtlCreate(c);
    mwOpaque_clear(co);
    co->data = o.data;
    co->len = o.len;
  }

  if(mwChannel_create(c))
    mw_raise("mwChannel_create error", NULL);

  return PyInt_FromLong(mwChannel_getId(c));
}


static PyObject *py_chan_destroy(mwPySession *self, PyObject *args) {
  guint32 id, reason = 0x00;
  struct mwChannel *c;

  /* XXX add opaque param */

  if(! PyArg_ParseTuple(args, "l|l", &id, &reason))
    return NULL;

  c = mwChannel_find(mwSession_getChannels(self->session), id);
  if(! c) mw_raise("no such channel", NULL);

  if(mwChannel_destroy(c, reason, NULL))
    mw_raise("error while destroying channel", NULL);

  mw_return_none();
}


static PyObject *py_chan_accept(mwPySession *self, PyObject *args) {
  guint32 id;
  struct mwChannel *c;
  struct mwOpaque o = { 0, 0 };
  struct mwOpaque *co;

  if(! PyArg_ParseTuple(args, "l|t#", &id, &o.data, &o.len))
    return NULL;

  c = mwChannel_find(mwSession_getChannels(self->session), id);
  if(! c) mw_raise("no such channel", NULL);

  co = mwChannel_getAddtlAccept(c);
  mwOpaque_clear(co);
  co->data = o.data;
  co->len = o.len;

  if(mwChannel_accept(c))
    mw_raise("error while accepting channel", NULL);

  mw_return_none();
}


static PyObject *py_chan_exists(mwPySession *self, PyObject *args) {
  guint32 id = 0;
  struct mwChannel *c;

  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  c = mwChannel_find(mwSession_getChannels(self->session), id);
  return PyInt_FromLong(c != NULL);
}


static PyObject *py_chan_send(mwPySession *self, PyObject *args) {
  guint32 id, type; 
  struct mwOpaque o = { 0, 0 };
  gboolean enc = TRUE;
  struct mwChannel *c;

  if(! PyArg_ParseTuple(args, "llt#|l", &id, &type, &o.data, &o.len, &enc))
    return NULL;

  c = mwChannel_find(mwSession_getChannels(self->session), id);
  if(! c) mw_raise("no such channel", NULL);

  if(mwChannel_sendEncrypted(c, type, &o, enc))
    mw_raise("error sending data on channel", NULL);

  mw_return_none();
}


static PyObject *py_chan_get_status(mwPySession *self, PyObject *args) {
  guint32 id = 0;
  struct mwChannel *c;
  enum mwChannelState state;

  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  c = mwChannel_find(mwSession_getChannels(self->session), id);
  if(! c) mw_raise("no such channel", NULL);

  state = mwChannel_getState(c);
  return PyInt_FromLong(state);
}


static PyObject *py_chan_get_service(mwPySession *self, PyObject *args) {
  guint32 id = 0;
  struct mwChannel *c;
  struct mwService *srvc;
  PyObject *py_srvc;
  
  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  c = mwChannel_find(mwSession_getChannels(self->session), id);
  if(! c) mw_raise("no such channel", NULL);

  srvc = mwChannel_getService(c);
  if(! srvc) mw_raise("channel has no service", NULL);

  py_srvc = mwService_getClientData(srvc);
  if(! py_srvc) mw_raise("service has no python wrapping", NULL);

  Py_INCREF(py_srvc);
  return py_srvc;
}


static PyObject *py_chan_get_protocol(mwPySession *self, PyObject *args) {
  guint32 id = 0;
  struct mwChannel *c;
  PyObject *t;
  
  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  c = mwChannel_find(mwSession_getChannels(self->session), id);
  if(! c) mw_raise("no such channel", NULL);

  t = PyTuple_New(2);
  PyTuple_SetItem(t, 0, PyInt_FromLong(mwChannel_getProtoType(c)));
  PyTuple_SetItem(t, 1, PyInt_FromLong(mwChannel_getProtoVer(c)));

  return t;
}


static PyObject *py_chan_get_user(mwPySession *self, PyObject *args) {
  guint32 id = 0;
  struct mwChannel *c;
  struct mwLoginInfo *i;
  PyObject *t;
  
  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  c = mwChannel_find(mwSession_getChannels(self->session), id);
  if(! c) mw_raise("no such channel", NULL);

  i = mwChannel_getUser(c);
  if(! i) mw_raise("no user for channel", NULL);

  t = PyTuple_New(2);
  PyTuple_SetItem(t, 0, PyString_SafeFromString(i->user_id));
  PyTuple_SetItem(t, 1, PyString_SafeFromString(i->community));

  return t;
}


static PyObject *py_add_service(mwPySession *self, PyObject *args) {
  struct mwSession *sess;
  mwPyService *srvcobj;
  struct mwService *srvc;

  if(! PyArg_ParseTuple(args, "O", &srvcobj))
    return NULL;

  if(! PyObject_IsInstance((PyObject *) srvcobj,
			   (PyObject *) mwPyService_type()))
    return NULL;

  sess = self->session;
  srvc = MW_SERVICE(srvcobj->wrapper);

  if(g_hash_table_lookup(self->services, srvc) ||
     ! mwSession_addService(sess, srvc)) {
    return PyInt_FromLong(0);
  }
      
  Py_INCREF(srvcobj);
  g_hash_table_insert(self->services, srvc, srvcobj);
  return PyInt_FromLong(1);
}


static PyObject *py_get_service(mwPySession *self, PyObject *args) {
  guint32 id = 0;
  struct mwService *srvc;
  mwPyService *srvcobj;

  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  srvc = mwSession_getService(self->session, id);
  if(! srvc) {
    mw_return_none();
  }

  srvcobj = g_hash_table_lookup(self->services, srvc);
  g_return_val_if_fail(srvcobj != NULL, NULL);

  Py_INCREF(srvcobj);
  return (PyObject *) srvcobj;
}


static PyObject *py_rem_service(mwPySession *self, PyObject *args) {
  guint32 id = 0;
  struct mwService *srvc;
  mwPyService *srvcobj;

  if(! PyArg_ParseTuple(args, "l", &id))
    return NULL;

  srvc = mwSession_removeService(self->session, id);
  if(! srvc) {
    mw_return_none();
  }

  srvcobj = g_hash_table_lookup(self->services, srvc);
  g_return_val_if_fail(srvcobj != NULL, NULL);

  g_hash_table_steal(self->services, srvc);
  return (PyObject *) srvcobj;
}


static struct PyMethodDef tp_methods[] = {
  /* intended to be overridden by handler methods */
  { ON_IO_WRITE, (PyCFunction) py_io_write, METH_VARARGS,
    "override to handle session's outgoing data writes" },

  { ON_IO_CLOSE, MW_METH_NOARGS_NONE, METH_NOARGS,
    "override to handle session's outgoing close request" },

  { ON_STATE_CHANGE, MW_METH_NOARGS_NONE, METH_NOARGS,
    "override to be notified on session state change" },

  { ON_SET_PRIVACY, MW_METH_NOARGS_NONE, METH_NOARGS,
    "override to be notified on session privacy change" },

  { ON_SET_USER, MW_METH_NOARGS_NONE, METH_NOARGS,
    "override to be notified on session user status change" },

  { ON_ADMIN, MW_METH_VARARGS_NONE, METH_VARARGS,
    "override to handle session's admin broadcast messages" },

  { ON_ANNOUNCE, MW_METH_VARARGS_NONE, METH_VARARGS,
    "override to handle announcement messages" },

  /* real methods */
  { "start", (PyCFunction) py_start, METH_VARARGS,
    "start the session" },

  { "stop", (PyCFunction) py_stop, METH_VARARGS,
    "stop the session" },

  { "recv", (PyCFunction) py_recv, METH_VARARGS,
    "pass incoming socket data to the session for processing" },

  { "addService", (PyCFunction) py_add_service, METH_VARARGS,
    "add a service to the sesssion" },

  { "getService", (PyCFunction) py_get_service, METH_VARARGS,
    "lookup a service by its ID" },

  { "removeService", (PyCFunction) py_rem_service, METH_VARARGS,
    "remove a service from the session by its ID" },

  { "channelCreate", (PyCFunction) py_chan_create, METH_VARARGS,
    "create a new channel for service with supplied version,"
    " optional target tuple and optional opaque data. Channel"
    " events will be passed to the supplied service for handling."
    " Returns the new channel's ID" },

  { "channelDestroy", (PyCFunction) py_chan_destroy, METH_VARARGS,
    "destroy a channel by ID" },

  { "channelAccept", (PyCFunction) py_chan_accept, METH_VARARGS,
    "accept a new incoming channel with optional opaque data." },

  { "channelExists", (PyCFunction) py_chan_exists, METH_VARARGS,
    "check that a channel exists by ID" },

  { "channelSend", (PyCFunction) py_chan_send, METH_VARARGS,
    "send data on a channel ID" },

  { "getChannelStatus", (PyCFunction) py_chan_get_status, METH_VARARGS,
    "get the status of a channel by ID" },

  { "getChannelService", (PyCFunction) py_chan_get_service, METH_VARARGS,
    "reference the service owning a channel ID" },

  { "getChannelProtocol", (PyCFunction) py_chan_get_protocol, METH_VARARGS,
    "tuple of protocol type and version for a channel ID" },

  { "getChannelUser", (PyCFunction) py_chan_get_user, METH_VARARGS,
    "tuple of user, community of identity on other side of a channel ID" },

  { NULL },
};


static void decref(PyObject *obj) {
  Py_XDECREF(obj);
}


static PyObject *tp_new(PyTypeObject *t, PyObject *args, PyObject *kwds) {
  struct pyObj_mwSession *self;

  self = (struct pyObj_mwSession *) t->tp_alloc(t, 0);
  if(self) {
    struct mwSession *s;

    struct mwSessionHandler *h;
    h = g_new0(struct mwSessionHandler, 1);

    h->io_write = mw_io_write;
    h->io_close = mw_io_close;
    h->clear = mw_clear;
    h->on_stateChange = mw_on_state;
    h->on_setPrivacyInfo = mw_on_privacy;
    h->on_setUserStatus = mw_on_user;
    h->on_admin = mw_on_admin;
    h->on_announce = mw_on_announce;

    self->session = s = mwSession_new(h);
    self->services = g_hash_table_new_full(g_direct_hash, g_direct_equal,
					   NULL, (GDestroyNotify) decref);

    mwSession_setClientData(s, self, NULL);
    
    mwSession_addCipher(s, mwCipher_new_RC2_40(s));
    mwSession_addCipher(s, mwCipher_new_RC2_128(s));
  }

  return (PyObject *) self;
}


static void tp_dealloc(struct pyObj_mwSession *self) {
  g_hash_table_destroy(self->services);

  mwCipher_free(mwSession_getCipher(self->session, mwCipher_RC2_40));

  mwSession_free(self->session);

  self->ob_type->tp_free((PyObject *) self);
}


static PyTypeObject pyType_mwSession = {
  PyObject_HEAD_INIT(NULL)
  0, /* ob_size */
  "_meanwhile.Session",
  sizeof(mwPySession),
  0, /* tp_itemsize */
  (destructor) tp_dealloc,
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
  "a Meanwhile client session",
  0, /* tp_traverse */
  0, /* tp_clear */
  0, /* tp_richcompare */
  0, /* tp_weaklistoffset */
  0, /* tp_iter */
  0, /* tp_iternext */
  tp_methods,
  0, /* tp_members */
  0, /* tp_getset */
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


PyTypeObject *mwPySession_type() {
  if(! py_type) {
    g_message("readying type mwPySession");
    py_type = &pyType_mwSession;
    Py_INCREF(py_type);
    PyType_Ready(py_type);
  }

  return py_type;
}


