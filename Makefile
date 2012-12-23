### Makefile --- 

## Author: Bad_ptr

all: xsetprop
	@echo "[DONE]"

xsetprop: xsetprop.c
	gcc $^ -o $@ `pkg-config --libs --cflags x11 xmu`

### Makefile ends here
