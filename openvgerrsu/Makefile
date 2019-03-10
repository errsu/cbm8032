
LIBFLAGS=-L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -lpthread
INCLUDEFLAGS=-I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -fPIC -I.

all:	shapedemo screensize

shapedemo:	shapedemo.c petascii.c libshapes.o shapes.h
	gcc -Wall $(INCLUDEFLAGS) -o shapedemo shapedemo.c petascii.c libshapes.o $(LIBFLAGS)

screensize:	screensize.c libshapes.o  shapes.h
	gcc -Wall $(INCLUDEFLAGS) -o screensize screensize.c libshapes.o $(LIBFLAGS)

libshapes.o:	libshapes.c shapes.h
	gcc -O2 -Wall $(INCLUDEFLAGS) -c libshapes.c

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
