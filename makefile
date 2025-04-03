.PHONY: clean dist

dist: clean
	tar -hzcf "$(CURDIR).tar.gz" hashtable/* jdis/* holdall/* makefile

clean:
	$(MAKE) -C jdis clean
