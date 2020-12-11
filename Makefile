
COMPILE_PREFIX=arm-linux-gnueabihf-
CC := $(COMPILE_PREFIX)gcc
C++ := $(COMPILE_PREFIX)g++
AR := $(COMPILE_PREFIX)ar

LIBPROP_TARGET :=libxprop.so
BIN_TARGET := xprop_srv
#BIN_TEST1 := test1 
#BIN_TEST2 := test2
BIN_PROP_SET := prop_set
BIN_PROP_GET := prop_get
CFLAGS := -g -Wall -O2 -fpic
CFLAGS += -I ./ -I private -L ./
LIB := -lpthread  -lxprop
.PHONY:all clean
#objs : $(LIBPROP_TARGET_OBJS) 

all : $(LIBPROP_TARGET) $(BIN_TARGET) $(BIN_TEST1)	$(BIN_TEST2) $(BIN_PROP_GET) $(BIN_PROP_SET)

system_properties.o : system_properties.cpp
	$(C++) $(CFLAGS) -fpic -c $< -o $@


LIBPROP_TARGET_OBJS =\
	system_properties.o 

$(LIBPROP_TARGET):$(LIBPROP_TARGET_OBJS)
	$(C++) -shared -o $@ $(LIBPROP_TARGET_OBJS)

property_service.o : property_service.cpp
	$(C++) $(CFLAGS) -c $< -o $@


BIN_OBJS =\
	property_service.o

$(BIN_TARGET) : $(BIN_OBJS)
	$(C++) $(CFLAGS) -o $@  $(BIN_OBJS) $(LIB)


%.o:%.cpp 
	$(C++) $(CFLAGS) -c $< -o $@

TEST1_OBJS=\
	system_properties_test.o

TEST2_OBJS=\
	system_properties_test2.o



$(BIN_TEST1): $(TEST1_OBJS) 
	$(C++) $(CFLAGS) -o  $@ $(TEST1_OBJS) $(LIB)

$(BIN_TEST2): $(TEST2_OBJS)
	$(C++) $(CFLAGS) -o  $@ $(TEST2_OBJS) $(LIB)

PROP_GET_OBJS =\
	get_prop.o
PROP_SET_OBJS =\
	set_prop.o

$(BIN_PROP_GET): $(PROP_GET_OBJS)
	$(C++) $(CFLAGS) -o  $@ $(PROP_GET_OBJS) $(LIB)
	
$(BIN_PROP_SET): $(PROP_SET_OBJS) 
	$(C++) $(CFLAGS) -o  $@ $(PROP_SET_OBJS) $(LIB)

clean:
	rm -rf *~ *.o *.d  $(TARGET) $(BIN_TARGET) $(LIBPROP_TARGET) $(BIN_TEST1) $(BIN_TEST2) $(BIN_PROP_GET) $(BIN_PROP_SET) 





