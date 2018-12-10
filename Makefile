PREFIX?=/
BINARY=backlight-tooler

all:
	$(CC) -c webcam.c
	$(CC) -c readconfig.c
	$(CC) main.c webcam.o readconfig.o -o $(BINARY)
install:
	mkdir -p $(PREFIX)/usr/bin/
	mkdir -p $(PREFIX)/usr/lib/systemd/user/
	mkdir -p $(PREFIX)/usr/lib/systemd/system/
	mkdir -p $(PREFIX)/etc/
	mkdir -p $(PREFIX)/usr/share/man/man1/
	cp BacklightTooler $(PREFIX)/usr/bin/
	cp tools/BacklightToolerChangePermissions.service $(PREFIX)/usr/lib/systemd/system/
	cp tools/BacklightTooler.{service,timer} $(PREFIX)/usr/lib/systemd/user/
	cp tools/BacklightToolerServiceToggle $(PREFIX)/usr/bin/
	cp tools/BacklightTooler.conf $(PREFIX)/etc/
	gzip -c tools/BacklightTooler.1 > $(PREFIX)/usr/share/man/man1/BacklightTooler.1.gz
types: types.vim
types.vim: *.h
	ctags --c-kinds=tc -o- *.h |\
		awk 'BEGIN{printf("syntax keyword Type\t")}\
		{printf("%s ", $$1)}END{print ""}' > $@
clean:
	rm -f BacklightTooler *.o
