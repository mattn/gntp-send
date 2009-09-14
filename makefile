bin/gntp-send : objs/gntp-send.o objs/md5.o objs/tcp.o objs/growl.o
	gcc $^ -o $@

objs/growl.o : source/growl.c
	gcc -I headers -Wall -c $< -o $@

objs/tcp.o : source/tcp.c
	gcc -I headers -Wall -c $< -o $@

objs/md5.o : source/md5.c
	gcc -I headers -Wall -c $< -o $@

objs/gntp-send.o : source/gntp-send.c
	gcc -I headers -Wall -c $< -o $@

clean : 
	rm -f bin/* objs/*
