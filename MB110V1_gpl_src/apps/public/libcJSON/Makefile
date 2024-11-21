LOCAL_SRC_FILES:= libcJSON.c

LOCAL_LIBS += -lm
LOCAL_CFLAGS += -I./

#we should use autoconf later.
LOCAL_CFLAGS +=

LIB_OBJECTS := $(LOCAL_SRC_FILES:.c=.o)

TARGET = libMPcJSON.so

.PHONY: all

all:${TARGET}

%.o:%.c
	${CC} -fPIC ${CFLAGS} ${LOCAL_CFLAGS} -c $< -o  $@

${TARGET}:${LIB_OBJECTS}
	${CC}  -shared ${LDFLAGS} ${LOCAL_LIBS} ${LIB_OBJECTS} -o $@

clean:
	rm -f libMPcJSON.so *.o
