CFLAGS = -Wall -fPIC
INCLUDES := -I/usr/include \
	    -I/usr/include/libdrm \
	    -I/usr/lib/glib-2.0/include \
	    -I/usr/include/glib-2.0 \
	    -I/usr/include/gstreamer-1.0
LDFLAGS := -L/lib -L/usr/lib
LIBS := -lgstapp-1.0 -lgstallocators-1.0 -lgstbase-1.0 -lgmodule-2.0 -lgobject-2.0 -lglib-2.0
LIBS += -lnx-drm-allocator -lnx-renderer -lkms -ldrm

CC := gcc

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

NAME := gstnxrenderer
LIB_TARGET := lib$(NAME).so
APP_TARGET := test_$(NAME)

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

$(LIB_TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(LIB_TARGET) -o $@ $^ $(LIBS)

all: $(LIB_TARGET)

install: $(LIB_TARGET)
	cp mm_types.h ../sysroot/include
	cp $^ ../sysroot/lib

.PHONY: clean

clean:
	rm -f *.o
	rm -f $(LIB_TARGET)
	rm -f $(APP_TARGET)
