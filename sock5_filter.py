import logging
import select
import socket
import struct
from socketserver import ThreadingMixIn, TCPServer, StreamRequestHandler

logging.basicConfig(level=logging.DEBUG)
SOCKS_VERSION = 5
PASS_ADDR = {}

class ThreadingTCPServer(ThreadingMixIn, TCPServer):
    pass


class SocksProxy(StreamRequestHandler):
    def handle(self):
        # Greeting header 2 bytes from a client
        header = self.connection.recv(2)
        version, nmethods = struct.unpack("!BB", header)

        # socks 5
        # == SOCKS_VERSION assert nmethods > 0
        # Get available methods
        methods = self.get_available_methods(nmethods)
        # print('init', version, nmethods, methods)

        # accept only USERNAME/PASSWORD auth
        if 0 not in set(methods):
            # close connection
            self.connection.sendall(struct.pack("!BB", SOCKS_VERSION, 255))
            self.server.close_request(self.request)
            print('close, version not 0')
            return

        # Send server choice
        self.connection.sendall(struct.pack("!BB", SOCKS_VERSION, 0))

        # request
        version, cmd, _, address_type = struct.unpack("!BBBB", self.connection.recv(4))
        # print('req', version, cmd, address_type)

        if address_type == 1:  # IPv4
            address = socket.inet_ntoa(self.connection.recv(4))
        elif address_type == 3:  # Domain name
            domain_length = self.connection.recv(1)[0]
            address = self.connection.recv(domain_length)
            if is_ad(str(address)):
               self.server.close_request(self.request)
               return
            # print('dl', domain_length, address)
            address = socket.gethostbyname(address)
            # print(' dn', address)
        port = struct.unpack('!H', self.connection.recv(2))[0]

        # reply
        try:
            if cmd == 1:  # CONNECT
                remote = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                remote.connect((address, port))
                bind_address = remote.getsockname()
                #logging.info('Connected to %s %s' % (address, port))
            else:
                self.server.close_request(self.request)
                print('close cmd not CONNECT')

            addr = struct.unpack("!I", socket.inet_aton(bind_address[0]))[0]
            port = bind_address[1]
            reply = struct.pack("!BBBBIH", SOCKS_VERSION, 0, 0, 1,
                                addr, port)

        except Exception as err:
            logging.error(err)
            # return connection refused error
            # reply = self.generate_failed_reply(address_type, 5)
            self.server.close_request(self.request)
            return

        self.connection.sendall(reply)

        # establish data exchange
        if reply[1] == 0 and cmd == 1:
            self.exchange_loop(self.connection, remote)

        self.server.close_request(self.request)

    def get_available_methods(self, n):
        methods = []
        for i in range(n):
            methods.append(ord(self.connection.recv(1)))
        return methods

    def exchange_loop(self, client, remote):
        client_to_remote = 0
        remote_to_client = 0
        to_continue = True
        while to_continue:
            # wait until client or remote is available for read
            r, w, e = select.select([client, remote], [], [])
            # print (r, w, e)
            if client in r:
              try:
                data = client.recv(4096)
                if len(data) <= 0:
                   break
                client_to_remote += len(data)
                if remote.send(data) <= 0:
                   break
              except:
                break

            if remote in r:
              try:
                data = remote.recv(4096)
                if len(data) <= 0:
                   break
                remote_to_client += len(data)
                if client.send(data) <= 0:
                   break
              except:
                break
        # print('data', client.fileno(), client_to_remote, remote.getpeername(), remote_to_client)

keywords = 'adkernel doubleverify adnxs casalemedia aralego amazon-adsystem doubleclick ad-score'
AD_FILTER = {}
for key in keywords.split():
    AD_FILTER[key] = 0

def is_ad(address):
  add = address.split('.')[-2]
  if add in AD_FILTER:
    AD_FILTER[add] += 1
    return True
  if address in PASS_ADDR:
    PASS_ADDR[address] += 1
  else:
    PASS_ADDR[address] = 0
    print(address)
  return False

if __name__ == '__main__':
    with ThreadingTCPServer(('127.0.0.1', 9011), SocksProxy) as server:
        server.serve_forever()
