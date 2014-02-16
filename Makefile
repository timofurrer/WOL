all:
	gcc -Wall -o wol src/wol.c

clean:
	rm -f wol
