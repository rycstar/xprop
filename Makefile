COMPILE_PREFIX=arm-linux-gnueabihf-
LIBPROP_TARGET :=libxprop.so
LIBPROP_CTRL_TARGET :=libxpropctrl.so
BIN_TARGET := xprop_srv
PROP_GET_BIN_TARGET := prop_get
PROP_SET_BIN_TARGET := prop_set
CC := $(COMPILE_PREFIX)gcc
AR := $(COMPILE_PREFIX)ar
RANLIB := $(COMPILE_PREFIX)ranlib
STRIP := $(COMPILE_PREFIX)strip

CFLAGS := -g -Wall -O2
BIN_CFLAGS := -g -Wall -O2
#BIN_CFLAGS += -L./ -ltty
CFLAGS += -I ./api -I ./

LDFLAGS := -L ./

LIBPROP_TARGET_OBJS =\
	prop_tree.o \
	prop_api.o

LIBPROP_CTRL_TARGET_OBJS = \
	service/prop_ctrl.o

BIN_SRC =\
	prop_tree.c \
	service/prop_service.c

PROP_GET_BIN_SRC = ./unit_test/get_prop.c

PROP_SET_BIN_SRC = ./unit_test/set_prop.c

all : $(LIBPROP_TARGET) $(LIBPROP_CTRL_TARGET) $(BIN_TARGET) $(PROP_GET_BIN_TARGET) $(PROP_SET_BIN_TARGET)

objs : $(LIBPROP_TARGET_OBJS) $(LIBPROP_CTRL_TARGET_OBJS)

prop_tree.o : prop_tree.c
	$(CC) $(CFLAGS) -fpic -c $< -o $@

prop_api.o : prop_api.c
	$(CC) $(CFLAGS) -fpic -c $< -o $@

service/prop_ctrl.o : service/prop_ctrl.c
	$(CC) $(CFLAGS) -fpic -c $< -o $@

clean:
	rm -rf *~ *.o *.d  $(TARGET) $(BIN_TARGET) $(PROP_GET_BIN_TARGET) $(PROP_SET_BIN_TARGET) $(LIBPROP_TARGET) $(LIBPROP_CTRL_TARGET) $(LIBPROP_CTRL_TARGET_OBJS)
install:
	@echo Nothing to be make

$(LIBPROP_TARGET) : $(LIBPROP_TARGET_OBJS)
	$(CC) -shared -o $@ $(LIBPROP_TARGET_OBJS)

$(LIBPROP_CTRL_TARGET) : $(LIBPROP_CTRL_TARGET_OBJS)
	$(CC) -shared -o $@ $(LIBPROP_CTRL_TARGET_OBJS)
#	$(STRIP) $@
$(BIN_TARGET) :
	$(CC) $(CFLAGS) -o $@  $(BIN_SRC)
#	$(STRIP) $@
$(PROP_GET_BIN_TARGET) :
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $(PROP_GET_BIN_SRC) -lxprop
$(PROP_SET_BIN_TARGET) :
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@  $(PROP_SET_BIN_SRC) -lxpropctrl
