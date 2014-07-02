CC      = gcc
LD	= ld
RANLIB	= ranlib
CFLAGS  = -Wall -O2 -MD -I ../../
LDFLAGS = -O2 -L. -L ../../libdune -lminibox -ldune -lpthread -T minibox.ld
OBJ     = main.o umm.o trap.o

all: libminibox.a minibox

libminibox.a: $(OBJ)
	$(AR) cru $(@) $(OBJ)
	$(RANLIB) $(@)

minibox: nullminibox.o libminibox.a ../../libdune/libdune.a
	$(CC) -o $(@) $(<) $(LDFLAGS)

clean:
	rm -f *.o test *.d minibox libminibox.a

-include *.d
