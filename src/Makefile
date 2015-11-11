MPICC   = mpicc
MPIEXEC = mpiexec
CFLAGS += -Wall

EJECUTABLE = tp3

.PHONY : all new clean run

all : $(EJECUTABLE) run

# Usamos gnu99 solamente para tener VLA's (variable-length array),
# vienen bien para crear el arreglo de usuarios/servers!
tp3 : tp3.c srv.c
	$(MPICC) $(CFLAGS) -std=gnu99 -o tp3 tp3.c srv.c

run : tp3
	$(MPIEXEC) -np 2 ./$(EJECUTABLE) '.' 2 50 500 >/dev/null

clean :
	rm -f $(EJECUTABLE)

test : tp3
	#$(MPIEXEC) -np 6 ./$(EJECUTABLE) >/dev/null
	#$(MPIEXEC) -np 12 ./$(EJECUTABLE) >/dev/null
	#$(MPIEXEC) -np 4 ./$(EJECUTABLE) x 30 0 0 - 10 0 0 >/dev/null
	#$(MPIEXEC) -np 6 ./$(EJECUTABLE) x 30 0 0 - 10 0 0 + 20 0 0 >/dev/null
	#$(MPIEXEC) -np 6 ./$(EJECUTABLE) x 30 50 0 - 10 50 50 + 20 0 0 >/dev/null
	#$(MPIEXEC) -np 30 ./$(EJECUTABLE) x 10 0 0 - 10 0 0 + 10 0 0 l 10 0 0 / 10 0 0 . 10 0 0 w 10 0 0 s 10 50 50  >/dev/null
	#$(MPIEXEC) -np 60 ./$(EJECUTABLE) >/dev/null
	$(MPIEXEC) -np 88 ./$(EJECUTABLE) a 10 0 0 \
                                       b 10 5 5 \
                                       c 10 10 10 \
                                       d 10 15 0 \
                                       e 10 0 5 \
                                       f 10 5 10 \
                                       g 10 10 0 \
                                       h 10 15 5 \
                                       i 10 0 10 \
                                       j 10 5 0 \
                                       k 10 10 5 \
                                       l 10 15 10 \
                                       h 10 0 0 \
                                       i 10 5 5 \
                                       j 10 10 10 \
                                       k 10 15 0 \
                                       l 10 0 5 \
                                       m 10 5 10 \
                                       n 10 10 0 \
                                       o 10 15 5 \
                                       p 10 0 10 \
                                       q 10 5 0 \
                                       r 10 10 5 \
                                       s 10 15 10 \
                                       t 10 0 0 \
                                       u 10 5 5 \
                                       v 10 10 10 \
                                       w 10 15 0 \
                                       x 10 0 5 \
                                       y 10 5 10 \
                                       z 10 10 0 \
                                       0 10 15 5 \
                                       1 10 0 10 \
                                       2 10 5 0 \
                                       3 10 10 5 \
                                       4 10 15 10 \
                                       5 10 0 0 \
                                       6 10 5 5 \
                                       7 10 10 10 \
                                       8 10 15 0 \
                                       9 10 0 5 \
                                       - 10 5 10 \
                                       + 10 10 0 \
                                       / 10 15 5 \
                                       . 10 50 50  >/dev/null

new : clean all
