import requests

headers = {}
data = ''
tun_srv = 'http://localhost:8888'
real_srv = 'http://localhost:8000'
import time
#r = requests.post('http://localhost:51234/', headers=headers, data=data)
#print ([r])

#r = requests.get(tun_srv, headers=headers)
while True:
    headers['X-Enproxy-Op'] = ''
    data = ''
    r = requests.post(tun_srv, headers=headers,data=data)
    #print ([r.content])
    if (r.content[0]=='p'):
    	for pa in r.content[1:].split('\n'):
    	    r = requests.get(real_srv+pa, headers=headers)
    	    data = r.content
    	    headers['X-Enproxy-Op']=pa
    	    requests.post(tun_srv, headers=headers,data=data)
    	    print('get from real server',pa, data[:100])
    time.sleep(1)