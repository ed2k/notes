#!/usr/bin/env python
# vim: set ts=4 sw=4 et:

import logging
import sys, os

try:
    from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
    from ConfigParser import ConfigParser
    from urlparse import urlparse, urlsplit, urlunsplit, parse_qsl
    from urllib import urlencode
    from urllib2 import Request
    from io import BytesIO
except ImportError:
    from http.server import HTTPServer, BaseHTTPRequestHandler
    from configparser import ConfigParser
    from urllib.parse import (urlparse, parse_qsl, urlencode,
        urlunsplit, urlsplit)
    from io import BytesIO
    from urllib.request import Request

from gzip import GzipFile
from json import loads

# so we can run without installing
#sys.path.append(os.path.abspath('../'))

from sanction import Client, transport_headers

ENCODING_UTF8 = 'utf-8'

def get_config():
    config = ConfigParser({}, dict)
    config.read('getlantern.ini')

    c = config._sections['sanction']
    if '__name__' in c:
        del c['__name__']

    if 'http_debug' in c:
        c['http_debug'] = c['http_debug'] == 'true'

    return config._sections['sanction']


logging.basicConfig(format='%(message)s')
l = logging.getLogger(__name__)
config = get_config()
g_redirect_uri = 'http://localhost/login/google'

class Handler(BaseHTTPRequestHandler):
    route_handlers = {
        '/': 'handle_root',
        '/login/google': 'handle_google_login',
        '/oauth2/google': 'handle_google',
    }
    def do_GET(self):
        url = urlparse(self.path)
        if url.path in self.route_handlers:
            getattr(self, self.route_handlers[url.path])(
            dict(parse_qsl(url.query)))
        else:
            self.send_response(404)

    def success(func):
        def wrapper(self, *args, **kwargs):
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.log_message(self.path)
            self.end_headers()
            return func(self, *args, **kwargs)
        return wrapper

    @success
    def handle_root(self, data):
        self.wfile.write('''
            login with: <a href='/oauth2/google'>Google</a>,
        '''.encode(ENCODING_UTF8))


    def _gunzip(self, data):
        s = BytesIO(data)
        gz = GzipFile(fileobj=s, mode='rb')
        return gz.read()


    def dump_response(self, data):
        for k in data:
            self.wfile.write('{0}: {1}<br>'.format(k,
                data[k]).encode(ENCODING_UTF8))

    def dump_client(self, c):
        for k in c.__dict__:
            self.wfile.write('{0}: {1}<br>'.format(k,
                c.__dict__[k]).encode(ENCODING_UTF8))
        self.wfile.write('<hr/>'.encode(ENCODING_UTF8))

    def handle_google(self, data):
        self.send_response(302)
        c = Client(auth_endpoint='https://accounts.google.com/o/oauth2/auth',
            client_id=config['google.client_id'])
        self.send_header('Location', c.auth_uri(
            scope=config['google.scope'], access_type='offline',
            redirect_uri=g_redirect_uri))
        self.end_headers()

    @success
    def handle_google_login(self, data):
        c = Client(token_endpoint='https://accounts.google.com/o/oauth2/token',
            resource_endpoint='https://www.googleapis.com/oauth2/v1',
            client_secret=config['google.client_secret'])
        c.request_token(code=data['code'],
            redirect_uri=g_redirect_uri)

        self.dump_client(c)
        data = c.request('/userinfo')
        self.dump_response(data)

        if hasattr(c, 'refresh_token'):
            rc = Client(token_endpoint=c.token_endpoint,
                client_id=c.client_id,
                client_secret=c.client_secret,
                resource_endpoint=c.resource_endpoint,
                token_transport='headers')

            rc.request_token(grant_type='refresh_token',
                refresh_token=c.refresh_token)
            self.wfile.write('<p>post refresh token:</p>'.encode(ENCODING_UTF8))
            self.dump_client(rc)



if __name__ == '__main__':
    l.setLevel(1)
    server_address = ('', 60780)
    server = HTTPServer(server_address, Handler)
    l.info('Starting server on %sport %s \nPress <ctrl>+c to exit' % server_address)
    server.serve_forever()

                                                            

D
A

