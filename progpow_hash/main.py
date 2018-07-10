from progpow import *

def to_hexstring(a):
  r = ''
  for i in xrange(32):
    v = (uint8array_getitem(a,i))
    r += '%02x' % v
  return r

def to_array(s):
  r = new_uint8array(len(s)/2)
  i = 0
  while len(s) > 0:
    h = s[:2]
    hex_int = int('0x'+h, 16)
    uint8array_setitem(r,i, hex_int)
    s = s[2:]
    i+=1
  return r

header = to_array(
    "000000206812f2a87241851756cb79524ddb813a9f83bb4bbcdda51abe07daeba50000001280c5e80b3472eb0f63f14e0c2edebbbe0409fb30598ca0a5ee3674f9c7f875f3dd07000000000000000000000000000000000000000000000000000000000055b6375b1c88011e00000000000000000000000000000000000000000000000018fc3d3100000000");
mix = to_array(
    "5883f27cabc838b98e7cd660c5ee91e8b1882bd17f9e428b5aa6bc951d5a16f8");
nonce = (0x313dfc18);
out = new_uint8array(32)

r = get_block_progpow_hash_from_mix(header, mix, nonce, out)

o = to_hexstring(out)
print (r, o)
print (o == "00000148b6715b3a32a7b36e5c820fcd4660acb72947ab9ebe4949134688066e")
#don't forget to call delete_


