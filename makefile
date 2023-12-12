src = $(wildcard src/*.c)
obj = $(src:.c=.o)

CFLAGS  += -Wall -std=c99
LDFLAGS += -lm -std=c99

# Optimizations
# CFLAGS  += -O3 -fgnu89-inline -std=c99 -march=native -mtune=native
# LDFLAGS += -O3 -std=c99 -march=native -mtune=native

# Debug
CFLAGS += -ggdb
LDFLAGS += -ggdb

magpie: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) magpie
