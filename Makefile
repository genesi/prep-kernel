TARGET_BIN=usr/sbin/which-blockdev
TARGET_SRC=which-blockdev.c

$(TARGET_BIN): $(TARGET_SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET_BIN)

.PHONY: clean
