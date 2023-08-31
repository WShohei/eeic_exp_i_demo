CC = gcc
BUILDDIR = build
SRCDIR = src
CFLGAS_MS = -Wall -pthread # lpthreadではない
CFLGAS_MC = -Wall -lm 
CFLGAS_C_SFU = -Wall -lm -pthread

# serversfu.cpp
TARGET_S_SFU = $(BUILDDIR)/serversfu
OBJS_S_SFU = $(SRCDIR)/serversfu.o

# clientfu.cpp
TARGET_C_SFU = $(BUILDDIR)/clientsfu
OBJS_C_SFU = $(SRCDIR)/clientsfu.o

# multiserver.c
TARGET_MS = $(BUILDDIR)/multiserver
OBJS_MS = $(SRCDIR)/multiserver.o

# multiclient.c
TARGET_MC = $(BUILDDIR)/multiclient
OBJS_MC = $(SRCDIR)/multiclient.o

# serversfu
$(TARGET_S_SFU): $(OBJS_S_SFU)
	$(CC) $(CFLGAS_MS) -o $@ $^

# clientsfu
$(TARGET_C_SFU): $(OBJS_C_SFU)
	$(CC) $(CFLGAS_C_SFU) -o $@ $^

# multiserver
$(TARGET_MS): $(OBJS_MS)
	$(CC) $(CFLGAS_MS) -L./rust_apis -lrust_apis -lgmp -lm -pthread -ldl  -o $@ $^ 

# multiclient
$(TARGET_MC): $(OBJS_MC)
	$(CC) $(CFLGAS_MC) -L./rust_apis -lrust_apis -lgmp -lm -pthread -ldl -o $@ $^

all : $(TARGET_S_SFU) $(TARGET_C_SFU) $(TARGET_MS) $(TARGET_MC)

.PHONY: tmpclean clean

tmpclean: 
	rm -f $(SRCDIR)/*~

clean: tmpclean
	rm -f $(SRCDIR)/*.o $(BUILDDIR)/*

