_progpow.so: p_wrap.o progpow_block_header_hash.o
	gcc -shared p_wrap.o progpow_block_header_hash.o -o _progpow.so

p_wrap.o: p_wrap.c
	gcc -fPIC -c p_wrap.c -I /usr/include/python2.7

#	g++ -fPIC -c progpow_block_header_hash.cpp -I /usr/include/python2.7 -fno-exceptions -fno-threadsafe-statics
progpow_block_header_hash.o: progpow_block_header_hash.cpp
	gcc -fPIC -c progpow_block_header_hash.c -I /usr/include/python2.7 -fno-exceptions


p_wrap.c: p.i
	swig -python p.i



clean:
	rm *.so
	rm *.o
	rm *.pyc
	rm p_wrap.c
	rm progpow.py
