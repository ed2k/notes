import requests

key = '32123456'
desturl = 'www.google.com:80'
pa = '/humans.txt'

headers = {'X-Enproxy-Id': key, 'X-Enproxy-Dest-Addr': desturl, 'X-Enproxy-Op': 'write',
   'Content-type':'application/octet-stream'}

data = 'GET ' + pa +'\n'
data += 'Host: '+ desturl
data += '''
User-Agent: curl/7.35.0
Accept: */*
Proxy-Connection: Keep-Alive
X-Forwarded-For: 127.0.0.1


''' 

cert= ('/home/a/.lantern/pt/flashlight-server/servercert.pem',
  '/home/a/.lantern/pt/flashlight-server/proxypk.pem')

cert=('/home/a/vps.pem','/home/a/vps_key.pem')
#cert=('/home/a/servercert.pem','/home/a/proxypk.pem')
cert=('/home/a/servercert.pem','/home/a/proxypk.pem')

srvaddr = 'https://vps:14377'
#srvaddr = 'https://localhost:14377'
#srvaddr = 'https://192.168.1.72:2558'
r = requests.post(srvaddr, headers=headers, data=data, cert=cert,
 verify=False)
print ([r.headers])
print ([r.content])

headers['X-Enproxy-Op']='read'
r = requests.get(srvaddr, headers=headers, cert=cert, verify=False)
print ([r.headers])
print ([r.content])


