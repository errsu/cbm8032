LIBFLAGS=-L/opt/vc/lib -lbrcmEGL -lbrcmGLESv2 -lbcm_host -lpthread
INCLUDEFLAGS=-I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads -fPIC -I.

all:	displaytest testserial screensize

serial2hdmi:	serial2hdmi.c petscii.c libgraphics.o graphics.h
	gcc -Wall $(INCLUDEFLAGS) -o serial2hdmi serial2hdmi.c petscii.c libgraphics.o $(LIBFLAGS)

displaytest:	displaytest.c petscii.c libgraphics.o graphics.h
	gcc -Wall $(INCLUDEFLAGS) -o displaytest displaytest.c petscii.c libgraphics.o $(LIBFLAGS)

testserial:	testserial.c
	gcc -Wall $(INCLUDEFLAGS) -o testserial testserial.c $(LIBFLAGS)

screensize:	screensize.c libgraphics.o  graphics.h
	gcc -Wall $(INCLUDEFLAGS) -o screensize screensize.c libgraphics.o $(LIBFLAGS)

libgraphics.o:	libgraphics.c graphics.h
	gcc -O2 -Wall $(INCLUDEFLAGS) -c libgraphics.c

clean:
	rm -f *.o *.inc *.so *.c~ *.h~
