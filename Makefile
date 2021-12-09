CC = gcc
CFLAGS = -std=gnu99
_SDEPS = 6s_types.h 6s_lib.h
BUILD_DIR = .
SRC_DIR = src
SDEPS = $(patsubst %,$(SRC_DIR)/%,$(_SDEPS))
_SERV = 6s_m 6s_a 6s_g 6s_e
SERV = $(patsubst %,$(BUILD_DIR)/%.exe,$(_SERV))
CLI = $(BUILD_DIR)/6c.exe

.PHONY: all cli serv clean

all: $(CLI) $(SERV)

$(BUILD_DIR)/6s_%.exe: $(SRC_DIR)/6s_%.c $(SDEPS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/6c.exe: $(SRC_DIR)/6c.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

#---------------------------------------------------------------

ADDR = 127.0.0.1
PORT = 12345

VERB = 4
CFG = cfg.txt

cli:
	./6c.exe $(ADDR) $(PORT)
serv:
	./6s_m.exe $(CFG) $(VERB)


#---------------------------------------------------------------

clean:
	rm -rf $(BUILD_DIR)/*.exe
	rm -f logs/*

#---------------------------------------------------------------