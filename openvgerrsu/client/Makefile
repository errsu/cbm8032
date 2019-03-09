INCLUDEFLAGS=-I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -I..
LIBFLAGS=-L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -lpthread  -ljpeg

all: shapedemo screensize

shapedemo:	shapedemo.c ../libshapes.o ../oglinit.o
	gcc -Wall $(INCLUDEFLAGS)  -o shapedemo shapedemo.c ../libshapes.o ../oglinit.o $(LIBFLAGS)

test:	shapedemo
	./shapedemo demo 5

screensize:	screensize.c ../libshapes.so
	gcc -Wall $(INCLUDEFLAGS) -o  screensize screensize.c ../libshapes.so $(LIBFLAGS)
