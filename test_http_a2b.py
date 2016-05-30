import requests

headers = {}
data = ''
tun_srv = 'http://bridge.x10host.com/a2b.php'
b2a_url = 'http://bridge.x10host.com/buf.html'
import time
#print ([r])
#headers['Cookie'] = '1; PHPSESSID=5ln05n7c4rt341bie5ffvonap6'
headers['User-Agent']= 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.111 Safari/537.36'

r = requests.get(tun_srv, headers=headers)
headers['Cookie'] = '1; '+r.headers['Set-Cookie']

while True:
    r = requests.get(b2a_url, headers=headers)
    print('cmd:' + r.content)
    headers["Content-type"] = "application/x-www-form-urlencoded"
    data = {'text':'aaabbb<br>aaa<br>from python', 'conn':'buf.html'}
    r = requests.post(tun_srv, headers=headers, data=data)

    time.sleep(1)
