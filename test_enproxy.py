import requests

headers = {'X-Enproxy-Id': '2234567', 'X-Enproxy-Dest-Addr': 'www.google.com:80', 'X-Enproxy-Op': 'write',
   'Content-type':'application/octet-stream'}
data = '''GET /humans.txt
Host: www.google.com
User-Agent: curl/7.35.0
Accept: */*
Proxy-Connection: Keep-Alive
X-Forwarded-For: 127.0.0.1


'''

r = requests.post('http://localhost:51234/', headers=headers, data=data)
print ([r])
headers['X-Enproxy-Op']='read'
r = requests.get('http://localhost:51234/', headers=headers)
print (r.content)
