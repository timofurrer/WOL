all:
	@gcc -Wall -o wol src/wol.c
	@echo "Compiled WOL source"

install:
	@cp wol /usr/local/bin/wol
	@echo "Copied WOL binary to /usr/local/bin/wol"

clean:
	rm -f wol
