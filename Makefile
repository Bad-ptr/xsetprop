## Copyright 2012 Constantin Kulikov
##
## Author: Constantin Kulikov (Bad_ptr) <zxnotdead@gmail.com>
## Date: 2013/07/09 21:11:22
## License: GPL either version 2 or any later version


all: xsetprop
	@echo "[DONE]"

xsetprop: xsetprop.c
	@echo "$(CC) $^ => $@"
	$(CC) $(CFLAGS) $^ `pkg-config --libs --cflags x11 xmu` -o $@


.PHONY: clean rebuild install uninstall

clean:
	@echo "rm xsetprop"
	@rm "xsetprop"

rebuild: clean all

install: all
	@echo "Installing xsetprop to /usr/local/bin"
	install -c -b -s xsetprop /usr/local/bin

uninstall:
	@echo "Removing /usr/local/bin/xsetprop"
	rm /usr/local/bin/xsetprop


### Makefile ends here
