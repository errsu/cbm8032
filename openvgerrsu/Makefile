
LIBFLAGS=-L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -ljpeg
INCLUDEFLAGS=-I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -fPIC
all:	library	

libshapes.o:	libshapes.c shapes.h
	gcc -O2 -Wall $(INCLUDEFLAGS) -c libshapes.c

oglinit.o:	oglinit.c
	gcc -O2 -Wall $(INCLUDEFLAGS) -c oglinit.c

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
	indent -linux -c 60 -brf -l 132  libshapes.c oglinit.c shapes.h

library: oglinit.o libshapes.o
	gcc $(LIBFLAGS) -shared -o libshapes.so oglinit.o libshapes.o
