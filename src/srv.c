#include "srv.h"

/*
 *  Ejemplo de servidor que tiene el "sí fácil" para con su
 *  cliente y no se lleva bien con los demás servidores.
 *
 */


bool todos_respondieron(bool respondieron[], bool servers_vivos[], int n, int yo){
    for(int i = 0; i < n; i++){
        if(respondieron[i] == false && servers_vivos[i] == true && i != yo)
            return false;
    }

    return true;
}

#define DD(x, ...) // fprintf(stderr, x, __VA_ARGS__)

void servidor(int mi_cliente, int n)
{
    MPI_Status status; int origen, tag;
    int me = (mi_cliente - 1) / 2;
    bool hay_pedido_local = false;
    bool listo_para_salir = false;

    unsigned int our_sequence_number, highest_sequence_number = 0;
    unsigned int outstanding_reply_count, se_despidieron;
    bool respondieron[n], chau_lista[n];
    bool reply_deferred[n];
    bool servers_vivos[n];
    int cant_servers_vivos = n;


    for(int i = 0; i < n; i++){
        reply_deferred[i] = false;
        servers_vivos[i] = true;
        respondieron[i] = false;
        chau_lista[i] = false;
    }

    while( ! listo_para_salir ) {
        unsigned int k;
        MPI_Recv(&k, 1, MPI_UNSIGNED, ANY_SOURCE, ANY_TAG, COMM_WORLD, &status);
        origen = status.MPI_SOURCE;
        tag = status.MPI_TAG;
        
        if (tag == TAG_PEDIDO) {
            assert(origen == mi_cliente);
            debug("Mi cliente solicita acceso exclusivo");
            assert(hay_pedido_local == false);
            hay_pedido_local = true;
            our_sequence_number = highest_sequence_number + 1;
            
            
            for(int i = 0; i < n; i++){
                respondieron[i] = !servers_vivos[i];
            }
            respondieron[me] = true;

            outstanding_reply_count = cant_servers_vivos - 1;

            DD("Soy %d, me llego un pedido -> %d\n", me, outstanding_reply_count);

            if (outstanding_reply_count == 0){
                debug("Dándole permiso (frutesco por ahora?)");
                DD("Soy %d, otorgo recurso... PEDIDO\n", me);
                MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
            }

            for(int i = 0; i < n; i++){
                int otro_server = 2*i;
                if (otro_server != 2*me  && servers_vivos[i]){
                    DD("Soy %d, mando REQUEST a %d\n", me, otro_server/2);
                    MPI_Send(&our_sequence_number, 1, MPI_UNSIGNED, otro_server, TAG_REQUEST, COMM_WORLD);
                }
            }
        } 

        else if (tag == TAG_REQUEST) {
            bool defer_it;
            highest_sequence_number = MAX(highest_sequence_number, k);
            defer_it = hay_pedido_local
                       && ((k > our_sequence_number)
                            || ((k == our_sequence_number) && (origen > (2*me))));

            DD("Soy %d, me llego un request de %d, hice %s\n", me, origen/2, defer_it? "defer": "no defer");
            if(defer_it){
                reply_deferred[origen/2] = true;
            } else {
                MPI_Send(NULL, 0, MPI_INT, origen, TAG_REPLY, COMM_WORLD);
            }
        }

        else if (tag == TAG_REPLY) {

            if (respondieron[origen/2] == false){
                outstanding_reply_count--;
                respondieron[origen/2] = true;
            }

            DD("Soy %d, me llego un REPLY de %d, me faltan %d\n", me, origen/2, outstanding_reply_count);

            if (outstanding_reply_count == 0){
                debug("Dándole permiso (frutesco por ahora?)");

                DD("Soy %d, otorgo recurso... REPLY\n", me);
                MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
            }
        }

        else if (tag == TAG_LIBERO) {
            DD("Soy %d, libero el recurso\n", me);
            assert(origen == mi_cliente);
            debug("Mi cliente libera su acceso exclusivo");
            assert(hay_pedido_local == true);
            hay_pedido_local = false;
            //outstanding_reply_count = cant_servers_vivos - 1;
            for (int i = 0; i < n; i++){
                if (reply_deferred[i]){
                    reply_deferred[i] = false;
                    int servidor = 2*i;
                    MPI_Send(NULL, 0, MPI_INT, servidor, TAG_REPLY, COMM_WORLD);
                }
            }
        }


        else if (tag == TAG_MEVOY) {

            DD("\nSoy %d, me entere que %d murio \n", me, origen/2);
            int server = origen/2;
            servers_vivos[server] = false;
            cant_servers_vivos--;

            if(hay_pedido_local && respondieron[server] == false){
                outstanding_reply_count--;
                respondieron[server] = true;
                fprintf(stderr, "ASDASDASDASDFASF %d\n", outstanding_reply_count);
                
                if (outstanding_reply_count == 0){
                    debug("Dándole permiso (frutesco por ahora?)");
                    DD("Soy %d, otorgo recurso... ME VOY\n", me);
                    MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
                }

            }

            if (chau_lista[server] == false){
                se_despidieron--;
                chau_lista[server] = true;
            }

            if(se_despidieron == 0)
                listo_para_salir = true;


            MPI_Send(NULL, 0, MPI_INT, origen, TAG_CHAU, COMM_WORLD);
        }

        else if (tag == TAG_CHAU) {

            int server = origen/2;
            if(chau_lista[server] == false){
                se_despidieron--;
                chau_lista[server] = true;
            }

            if(se_despidieron == 0)
                listo_para_salir = true;

        }

        
        else if (tag == TAG_TERMINE) {
            assert(origen == mi_cliente);
            debug("Mi cliente avisa que terminó");
            // listo_para_salir = true;
            
            servers_vivos[me] = false;
            se_despidieron = cant_servers_vivos -1 ;

            if (se_despidieron == 0){
                listo_para_salir = true;
            }


            /* Les mando a todos los replies diferidos */
            for (int i = 0; i < n; i++){
                if (reply_deferred[i]){
                    reply_deferred[i] = false;
                    int servidor = 2*i;
                    MPI_Send(NULL, 0, MPI_INT, servidor, TAG_REPLY, COMM_WORLD);
                }
            }
            
            /* Les notifico a todos que me voy a ir */
            for (int i = 0; i < n; i++){
                if (servers_vivos[i]){
                    int servidor = 2*i;
                    MPI_Send(NULL, 0, MPI_INT, servidor, TAG_MEVOY, COMM_WORLD);
                }
            }

    
        }
        
    }
    DD("> SOY %d y MORI\n", me);
}
