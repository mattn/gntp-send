gntp-send : gntp-send.o
	gcc gntp-send.o -o gntp-send

gntp-send.o : gntp-send.c
	gcc -Wall -c gntp-send.c -o gntp-send.o

clean : 
	rm -f gntp-send gntp-send.o
