PREFIX?=/
BINARY:=backlight-tooler
CXX:=g++
LD:=g++
CXXFLAGS:=-O3 -Wall -Wextra -pedantic -std=c++11
LDFLAGS:=

.PHONY: all install types clean

SOURCE:=$(wildcard *.cpp)
OBJECTS:=$(patsubst %.cpp,%.o,$(SOURCE))

all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o $@

-include *.d

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

install:
	mkdir -p $(PREFIX)/usr/bin/
	mkdir -p $(PREFIX)/usr/lib/systemd/user/
	mkdir -p $(PREFIX)/usr/lib/systemd/system/
	mkdir -p $(PREFIX)/etc/
	mkdir -p $(PREFIX)/usr/share/man/man1/
	mkdir -p $(PREFIX)/usr/lib/udev/rules.d/
	cp backlight-tooler $(PREFIX)/usr/bin/
	cp tools/backlight-tooler.{service,timer} $(PREFIX)/usr/lib/systemd/user/
	cp tools/backlight-tooler-service-toggle $(PREFIX)/usr/bin/
	cp tools/backlight-tooler.conf $(PREFIX)/etc/
	cp tools/99-backlight-tooler-permissions.rules $(PREFIX)/usr/lib/udev/rules.d/
	gzip -c tools/backlight-tooler.1 > $(PREFIX)/usr/share/man/man1/backlight-tooler.1.gz
types: types.vim
types.vim: *.h
	ctags --c-kinds=tc -o- *.h |\
		awk 'BEGIN{printf("syntax keyword Type\t")}\
		{printf("%s ", $$1)}END{print ""}' > $@
clean:
	-$(RM) $(BINARY) $(OBJECTS) *.d
