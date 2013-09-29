CC = gcc

CFLAGS = -Wall -g -I../ -I/usr/include/freetype2
LDFLAGS = -L../egui/.libs/ -L../egal/.libs -L../elib/.libs -lelib -legui -legal -lfreetype -lXrender
ALL: button fixed label text clist anim box scrollwin menu menu_entrys entry frame notebook filesel ddlist hello bn image.o

bn: bn.c image.o
	${CC} ${CFLAGS} $^ -o $@ ${LDFLAGS}

image.o: image.c
	$(CC) $(CFLAGS) $^

hello: hello.c
	$(CC) $(CFLAGS) -o hello $^ $(LDFLAGS)

ddlist: ddlist.c
	$(CC) $(CFLAGS) -o ddlist $^ $(LDFLAGS)

filesel: filesel.c
	$(CC) $(CFLAGS) -o filesel $^ $(LDFLAGS)

notebook: notebook.c
	$(CC) $(CFLAGS) -o notebook $^ $(LDFLAGS)

frame: frame.c
	$(CC) $(CFLAGS) -o frame $^ $(LDFLAGS)

entry: entry.c
	$(CC) $(CFLAGS) -o entry $^ $(LDFLAGS)

menu_entrys: menu_entrys.c
	$(CC) $(CFLAGS) -o menu_entrys $^ $(LDFLAGS)

menu: menu.c
	$(CC) $(CFLAGS) -o menu $^ $(LDFLAGS)

scrollwin: scrollwin.c
	$(CC) $(CFLAGS) -o scrollwin $^ $(LDFLAGS)

box: box.c
	$(CC) $(CFLAGS) -o box $^ $(LDFLAGS)

anim: anim.c
	$(CC) $(CFLAGS) -o anim $^ $(LDFLAGS)

clist: clist.c
	$(CC) $(CFLAGS) -o clist $^ $(LDFLAGS)

button: button.c
	$(CC) $(CFLAGS) -o button $^ $(LDFLAGS)

label: label.c
	$(CC) $(CFLAGS) -o label $^ $(LDFLAGS)

text: text.c
	$(CC) $(CFLAGS) -o text $^ $(LDFLAGS)

fixed: fixed.c
	$(CC) $(CFLAGS) -o fixed $^ $(LDFLAGS)

clean:
	rm -f label text button clist anim box frame entry notebook menu menu_entrys scrollwin fixed ddlist filesel
