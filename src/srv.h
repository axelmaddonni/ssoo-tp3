#ifndef __srv_h__
#define __srv_h__

#include "tp3.h"
#include <stdbool.h>

void servidor(int mi_cliente, int n);

#define TAG_REQUEST 70
#define TAG_REPLY   80
#define TAG_MEVOY   90
#define TAG_CHAU    100

#define MAX(a,b) ((a)>(b))?(a):(b)



#endif
