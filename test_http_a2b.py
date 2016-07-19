import requests

headers = {}
data = ''
tun_srv = 'http://bridge.x10host.com/a2b.php'
a2b_url = 'http://bridge.x10host.com/a2b.html'

import time
#print ([r])
#headers['Cookie'] = '1; PHPSESSID=5ln05n7c4rt341bie5ffvonap6'
headers['User-Agent']= 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_2) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.111 Safari/537.36'

#r = requests.get(tun_srv, headers=headers)
#headers['Cookie'] = '1; '+r.headers['Set-Cookie']

old_cmd = 'nothing'
while True:
    r = requests.get(a2b_url, headers=headers)
    cmd = r.content
    if old_cmd == cmd:
        r = None
        print('old',cmd)
        time.sleep(11)
        continue
    old_cmd = cmd
    print('cmd:' + cmd)
    headers["Content-type"] = "application/x-www-form-urlencoded"
    data = {'text':'recved cmd '+cmd, 'conn':'buf.html'}
    r = requests.post(tun_srv, headers=headers, data=data)

    time.sleep(13)
