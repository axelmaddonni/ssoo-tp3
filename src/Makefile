MPICC   = mpicc
MPIEXEC = mpiexec
CFLAGS += -Wall

EJECUTABLE = tp3

.PHONY : all new clean run

all : $(EJECUTABLE) run

tp3 : tp3.c srv.c
	$(MPICC) $(CFLAGS) -std=gnu99 -o tp3 tp3.c srv.c

run : tp3
	$(MPIEXEC) -np 2 ./$(EJECUTABLE) '.' 2 50 500 >/dev/null

clean :
	rm -f $(EJECUTABLE)

test : tp3
	:

new : clean all
