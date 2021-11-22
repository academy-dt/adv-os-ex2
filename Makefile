SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

CFLAGS = -Werror -Wall
LDFLAGS =

timer.out: $(OBJ)
	        $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
	    clean:
		        rm -f $(OBJ) piper.out
