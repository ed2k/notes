#!/usr/bin/env python
""" simple HTTP tunnel server in python.

Usage::
    ./dummy-web-server.py [<port>]

Send a GET request::
    curl http://localhost

Send a HEAD request::
    curl -I http://localhost

Send a POST request::
    curl -d "foo=bar&bin=baz" http://localhost

"""
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import SocketServer
import time


class S(BaseHTTPRequestHandler):
    user_getbuf = []
    realsrv_buf = {}
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        self._set_headers()
        real_buf = self.realsrv_buf
        if self.path not in self.user_getbuf:
            self.user_getbuf.append(self.path)
        print ('from user', self.user_getbuf)
        
        if self.path in real_buf and len(real_buf[self.path])!=0:
            print ('reply to user', real_buf[self.path])
            self.wfile.write(''.join(real_buf[self.path]))
            real_buf.pop(self.path, None)
        else:
            self.wfile.write("<html><body><h1>retry...</h1></body></html>")
        print('endofget')

    def do_HEAD(self):
        self._set_headers()
        
    def do_POST(self):
        # Doesn't do anything with posted data
        print(''.join(str(self.headers).split('\n')), self.user_getbuf)
        
        
        h = self.headers
        self._set_headers()
        if 'X-Enproxy-Op' in h:
            pa = h['X-Enproxy-Op']
            if pa not in self.realsrv_buf: self.realsrv_buf[pa] = []
            if 'Content-Length' in h:
              postlen = int(h['Content-Length'])
              if postlen>0:
                self.realsrv_buf[pa].append(self.rfile.read(postlen))
                print('gotfrom realsrv', self.realsrv_buf)
            else:
                self.realsrv_buf[pa].append(self.rfile.read())
                print('chunked from realsrv', self.realsrv_buf)
            if len(self.user_getbuf)!=0:                
                self.wfile.write('p'+'\n'.join(self.user_getbuf))
                del self.user_getbuf[:]
                print('--------- sent user cmd ----------',self.user_getbuf)
            else: self.wfile.write('b')


def run(server_class=HTTPServer, handler_class=S, port=80):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print 'Starting httpd...'
    httpd.serve_forever()

if __name__ == "__main__":
    from sys import argv

    if len(argv) == 2:
        run(port=int(argv[1]))
    else:
        run()
