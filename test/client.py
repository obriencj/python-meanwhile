##############################################################################
# simple python gtk client test script
##############################################################################


import gobject
import gtk
import meanwhile
import os
import sys



class ClientServiceAware(meanwhile.ServiceAware):
    
    def __init__(self, session):
        meanwhile.ServiceAware.__init__(self, session)
        
        store = self._make_store()

        view = self._make_view(store)
        view.connect("button-press-event", self._press_cb)

        scroll = gtk.ScrolledWindow()
        scroll.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        scroll.add(view)
        
        window = gtk.Window()
        window.set_title("Meanwhile")
        window.connect("destroy", self._destroy_cb)
        window.resize(150, 300)

        window.add(scroll)
        
        self.window = window
        self.store = store
        self.view = view


    def _double_click_cb(self, widget):
        sel = widget.get_selection()
        iter = sel.get_selected()[1]
        
        if not iter:
            return
        
        id = self.store.get_value(iter, 0)
        who = (id, None)
        window = self.session.srvc_im._ensure_convo(who)
        window.show_all() 


    def _press_cb(self, widget, button, *k):
        if button.type == 5:
            self._double_click_cb(widget)
            

    def _destroy_cb(self, *kw):
        self.session.stop()


    def _make_store(self):
        store = gtk.TreeStore(gobject.TYPE_STRING, gobject.TYPE_STRING)
        return store
    

    def _make_view(self, model):
        view = gtk.TreeView(model)
        
        render0 = gtk.CellRendererText()
        col0 = gtk.TreeViewColumn("User", render0, text=0)
        view.append_column(col0)

        render1 = gtk.CellRendererText()
        col1 = gtk.TreeViewColumn("Status", render1, text=1)
        view.append_column(col1)

        return view
        

    def start(self):
        self.window.show_all()
        meanwhile.ServiceAware.start(self)


    def stop(self):
        self.store.clear()
        meanwhile.ServiceAware.stop(self)


    def _zoom_check(self, iter, id):
        print "_zoom_check", iter, id
        zid = self.store.get_value(iter, 0)
        return zid < id[0]


    def _zoom_to(self, id):
        store = self.store
        
        iter = store.get_iter_first()
        while iter and self._zoom_check(iter, id):
            iter = store.get_iter_next(iter)

        return iter

        
    def _add(self, id):
        iter = self._zoom_to(id)
        data = [id[0], 'Offline']
        
        if not iter:
            self.store.append(None, data)
        elif self.store.get_value(iter, 0) != id[0]:
            self.store.insert_before(None, iter, data)


    def add(self, id):
        if meanwhile.ServiceAware.add(self, id):
            for i in id:
                self._add(i)


    def _remove(self, id):
        iter = self._zoom_to(id)

        if iter and id[0] == self.store.get_value(iter, 0):
            self.store.remove(iter)
    

    def remove(self, id):
        if meanwhile.ServiceAware.remove(self, id):
            for i in id:
                self._remove(id)

    
    def onAware(self, id, stat):
        iter = self._zoom_to(id)
        if iter and id[0] == self.store.get_value(iter, 0):
            self.store.set_value(iter, 1, stat[2])



class ClientServiceIm(meanwhile.ServiceIm):

    def __init__(self, session):
        meanwhile.ServiceIm.__init__(self, session)
        self._send_queue = {}
        self._convos = {}

        
    def _queue(self, who, cb, data):
        
        ''' queues up a send call to be handled when the conversation
        is fully opened. '''
        
        q = self._send_queue
        t = (cb, data)
        if q.has_key(who):
            q[who].append(t)
        else:
            q[who] = [t]


    def _delqueue(self, who):
        
        ''' deletes all queued send calls '''
        
        q = self._send_queue
        if q.has_key(who):
            del q[who]


    def _runqueue(self, who):
        
        ''' sends all queued calls '''
        
        q = self._send_queue
        if q.has_key(who):
            for act in q[who]:
                # don't let an exception invalidate the rest of the queue
                try:
                    act[0](self, who, act[1])
                except Exception, e:
                    print >> sys.stderr, "exception clearing IM queue: %s" % e
            del q[who]


    def _open_convo(self, who):
        
        buffer = gtk.TextBuffer()
                
        output = gtk.TextView(buffer)
        output.set_wrap_mode(gtk.WRAP_WORD)
        output.set_editable(gtk.FALSE)
        output.set_cursor_visible(gtk.FALSE)
        output.unset_flags(gtk.CAN_FOCUS)

        scroll = gtk.ScrolledWindow()
        scroll.set_policy(gtk.POLICY_NEVER, gtk.POLICY_ALWAYS)
        scroll.unset_flags(gtk.CAN_FOCUS)
        scroll.add(output)
        
        input = gtk.Entry()
        input.set_flags(gtk.CAN_FOCUS)

        button = gtk.Button("send")
        button.connect_object("clicked", self._on_send, input, who)
        button.set_flags(gtk.CAN_DEFAULT)

        hbox = gtk.HBox()
        hbox.pack_start(input)
        hbox.pack_end(button, expand=gtk.FALSE)

        vbox = gtk.VBox()
        vbox.pack_start(scroll)
        vbox.pack_end(hbox, expand=gtk.FALSE)        

        window = gtk.Window()
        window.set_title(who[0])
        window.connect("destroy", self._destroy_cb)
        window.resize(300, 200)
        window.add(vbox)

        window.mw_user = who
        window.mw_buffer = buffer
        window.mw_view = output

        self._convos[who] = window
        window.show_all()

        button.grab_default()
        input.grab_focus()
        
        return window


    def _ensure_convo(self, who):
        print "_ensure_convo", who
        
        window = self._convos.get(who)
        if not window:
            window = self._open_convo(who)
        return window


    def _on_send(self, widget, who):
        txt = widget.get_text().strip()
        if txt:
            self.sendText(who, txt)
            window = self._ensure_convo(who)
            self._append_convo(window, self.session._id, txt)

        widget.set_text('')
        widget.grab_focus()


    def _destroy_cb(self, win, *k):
        who = win.mw_user
        
        self.closeConversation(who)
        del self._convos[who]
        

    def onOpened(self, who):
        print '<opened>', who
        self._runqueue(who)
        self._ensure_convo(who)

        who = (who[0], who[1], meanwhile.AWARE_USER)
        self.session.srvc_aware.add([who])
        

    def onClosed(self, who, err):
        print '<closed>%s: 0x%x' % (who[0], err)
        self._delqueue(who)


    def _append_convo(self, window, who, text):

        buf = window.mw_buffer
        end = buf.get_end_iter()
        buf.insert(end, "%s : %s\n" % (who[0], text))

        mark = buf.get_mark('end')
        if mark:
            buf.move_mark(mark, end)
        else:
            mark = buf.create_mark('end', end, left_gravity=gtk.FALSE)
        
        window.mw_view.scroll_mark_onscreen(mark)
        

    def sendText(self, who, text):
        state = self.conversationState(who)
        if state == meanwhile.CONVERSATION_OPEN:
            meanwhile.ServiceIm.sendText(self, who, text)
        else:
            self._queue(who, meanwhile.ServiceIm.sendText, text)
            if state == meanwhile.CONVERSATION_CLOSED:
                self.openConversation(who)


    def sendHtml(self, who, html):
        state = self.conversationState(who)
        if state == meanwhile.CONVERSATION_OPEN:
            meanwhile.ServiceIm.sendHtml(self, who, html)
        else:
            self._queue(who, meanwhile.ServiceIm.sendHtml, html)
            if state == meanwhile.CONVERSATION_CLOSED:
                self.openConversation(who)


    def sendSubject(self, who, subj):
        state = self.conversationState(who)
        if state == meanwhile.CONVERSATION_OPEN:
            meanwhile.ServiceIm.sendSubject(self, who, subj)
        else:
            self._queue(who, meanwhile.ServiceIm.sendSubject, subj)
            if state == meanwhile.CONVERSATION_CLOSED:
                self.openCconversation(who)


    def sendTyping(self, who, typing=True):
        state = self.conversationState(who)
        if state == meanwhile.CONVERSATION_OPEN:
            meanwhile.ServiceIm.sendTyping(self, who, typing)
        else:
            self._queue(who, meanwhile.ServiceIm.sendTyping, typing)
            if state == meanwhile.CONVERSATION_CLOSED:
                self.openConversation(who)


    def onText(self, who, text):
        print '<text>%s: "%s"' % (who[0], text)
        window = self._ensure_convo(who)
        self._append_convo(window, who, text)


    def onHtml(self, who, text):
        print '<html>%s: "%s"' % (who[0], text)


    def onMime(self, who, data):
        
        ''' Handles incoming MIME messages, such as those sent by
        NotesBuddy containing embedded image data '''

        from email.Parser import Parser
        from StringIO import StringIO
        
        print '<mime>%s' % who[0]
        msg = Parser().parsestr(data)

        html = StringIO()  # combined text segments
        images = {}        # map of Content-ID:binary image
        
        for part in msg.walk():
            mt = part.get_content_maintype()
            if mt == 'text':
                html.write(part.get_payload(decode=True))
            elif mt == 'image':
                cid = part.get('Content-ID')
                images[cid] = part.get_payload(decode=True)

        print ' <text/html>:', html.getvalue()
        html.close()

        print ' <images>:', [k[1:-1] for k in images.keys()]
                

    def onSubject(self, who, subj):
        print '<subject>%s: "%s"' % (who[0], subj)


    def onTyping(self, who, typing):
        str = ("stopped typing", "typing")
        print '<typing>%s: %s' % (who[0], str[typing])



class Client(meanwhile.SocketSession):
    
    def __init__(self, who, where):
        meanwhile.SocketSession.__init__(self, who, where)

        self.srvc_aware = ClientServiceAware(self)
        self.addService(self.srvc_aware)
        
        self.srvc_im = ClientServiceIm(self)
        self.addService(self.srvc_im)
        
        # self.srvc_resolve = meanwhile.ServiceResolve(self)
        # self.addService(self.srvc_resolve)
        
        # self.srvc_store = meanwhile.ServiceStorage(self)
        # self.addService(self.srvc_store)


    def _recv_cb(self, sock, cond):
        if cond == gobject.IO_IN:
            data = self._sock.recv(1024)
            self.recv(data)
            return True
            
        else:
            self.stop(-1)
            return False


    def _recvLoop(self):
        conds = gobject.IO_IN + gobject.IO_HUP
        self.tag = gobject.io_add_watch(self._sock, conds, self._recv_cb)


    def start(self):
        meanwhile.SocketSession.start(self, background=False, daemon=False)
        

    def stop(self, reason=0):
        gobject.source_remove(self.tag)
        self.tag = None
        
        meanwhile.SocketSession.stop(self, reason)
        
        gtk.main_quit()



def main(argv):
    ''' '''

    test_user = os.environ.get('mw_user')
    test_pass = os.environ.get('mw_pass')
    test_host = os.environ.get('mw_host')
    test_port = int(os.environ.get('mw_port'))

    client = Client((test_host, test_port), (test_user, test_pass))
    gobject.idle_add(lambda c=client: c.start())

    gtk.main()



if __name__ == '__main__':
#    try:
        main(sys.argv)
#        
#    except Exception, e:
#        print >> sys.stderr, e
#        sys.exit(1)
    

