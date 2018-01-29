OBJECTS=button fixed label text clist anim box scrollwin menu menu_entrys entry frame notebook filesel ddlist hello gis opengl image.o

CC = gcc

CFLAGS=-Wall -g -I../ -I/usr/include/freetype2
LDFLAGS= -L../egui/.libs/ -L../egal/.libs -L../elib/.libs -legui -legal -lelib -lfreetype -lfontconfig -lXrender -lX11 -lGL -lGLU -lpng -lm -ljpeg -lpthread 
ALL: $(OBJECTS)

opengl: opengl.c image.o
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

hello: hello.c image.o
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

image.o: image.c
	$(CC) $(CFLAGS) $^ -c

gis: gis.c
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

ddlist: ddlist.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
filesel: filesel.c     
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
notebook: notebook.c   
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
frame: frame.c         
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
entry: entry.c         
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

menu_entrys: menu_entrys.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
menu: menu.c           
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

scrollwin: scrollwin.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
box: box.c             
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
anim: anim.c           
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
clist: clist.c         
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
button: button.c       
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
label: label.c         
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
                       
text: text.c           
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

fixed: fixed.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm $(OBJECTS)
