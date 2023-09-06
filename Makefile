
%.o: %.c exyflash.c
	$(CC) $< -o $@ -lusb-1.0 -static --static -c

exyflash: exyflash.o
	$(CC) $< -o $@ -lusb-1.0 -static --static

.PHONY: clean
clean:
	rm -rfv exyflash.o exyflash
