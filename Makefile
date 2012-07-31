### Makefile --- 

## Author: Bad_ptr

all: xsetprop
	@echo "[DONE]"

xsetprop: xsetprop.c
	gcc `pkg-config --libs --cflags x11 xmu` $^ -o $@

### Makefile ends here
