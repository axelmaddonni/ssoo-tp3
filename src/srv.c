#include "srv.h"

/* Cuando un server muere:
 * Enviamos un mensaje a todos los servidores con tag TAG_MEVOY, y esperamos a que todos contesten
 * con un TAG_CHAU. En el interim, seguimos contestando todos los REQUESTs y MEVOYs entrantes, hasta
 * que todos nos contesten. Cuando todos los servers nos contestaron, nos salimos del loop y 
 * terminamos la ejecucion.
 */

  

#define DD(x, ...) // fprintf(stderr, x, __VA_ARGS__)

void servidor(int mi_cliente, int n)
{
    MPI_Status status; int origen, tag;
    int me = (mi_cliente - 1) / 2;
    bool hay_pedido_local = false;
    bool listo_para_salir = false;


    // Estas son las mismas variables que las del paper.
    // se_despidieron es la cantidad de servidores que respondieron CHAU luego de me_voy
    // respondieron[] es la lista de servidores, y si el servidor iesimo mando un REPLY,
    //        respondieron[i] == true
    // chau_lista[] es lo mismo que respondieron, pero para CHAU
    // servers_vivos[] es un arreglo que indica si cada server murio o no
    unsigned int our_sequence_number, highest_sequence_number = 0;
    unsigned int outstanding_reply_count, se_despidieron;
    bool respondieron[n], chau_lista[n];
    bool reply_deferred[n];
    bool servers_vivos[n];
    int cant_servers_vivos = n;


    for(int i = 0; i < n; i++){
        // inicializamos todo
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
                // los servidores muertos ya respondieron (REPLY), y los vivos todavia no
                respondieron[i] = !servers_vivos[i];
            }
            // self no tiene que responderse
            respondieron[me] = true;

            // la cantidad de servidores que deben responder son los vivos - 1 
            // (porque self no tiene que responderse). cantidad de posiciones en false de respondieron
            outstanding_reply_count = cant_servers_vivos - 1;

            DD("Soy %d, me llego un pedido -> %d\n", me, outstanding_reply_count);

            if (outstanding_reply_count == 0){
                // si estoy en 0, significa que soy el unico server vivo, entonces el recurso
                // es nuestro. 
                debug("Dándole permiso (frutesco por ahora?)");
                DD("Soy %d, otorgo recurso... PEDIDO\n", me);
                MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
            }

            for(int i = 0; i < n; i++){
                // le mando REQUEST a todos los servers vivos
                // los servers son los numeros pares de 0..2n
                int otro_server = 2*i;
                if (otro_server != 2*me  && servers_vivos[i]){
                    DD("Soy %d, mando REQUEST a %d\n", me, otro_server/2);
                    MPI_Send(&our_sequence_number, 1, MPI_UNSIGNED, otro_server, TAG_REQUEST, COMM_WORLD);
                }
            }
        } 

        else if (tag == TAG_REQUEST) {
            // Esto está copiado del paper.
            // De nuevo, 2*me porque el server es el numero par que corresponde a ese nodo
            // origen/2  por la misma razon
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
            // si todavia no respondio (puede haber muerto), lo marco como respondido y
            // disminuyo la cantidad de gente que estoy esperando
            if (respondieron[origen/2] == false){
                outstanding_reply_count--;
                respondieron[origen/2] = true;
            }

            DD("Soy %d, me llego un REPLY de %d, me faltan %d\n", me, origen/2, outstanding_reply_count);

            // si llegue a 0, listo, el recurso es mio.
            if (outstanding_reply_count == 0){
                debug("Dándole permiso (frutesco por ahora?)");

                DD("Soy %d, otorgo recurso... REPLY\n", me);
                MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
            }
        }

        else if (tag == TAG_LIBERO) {
            // Esto es lo mismo que en el paper, envio reply a todos los que habia diferido
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
            // si estoy aca, me llego la notificacion de que un server se esta cerrando
            // lo marco como muerto, y disminuyo la cantidad de servidores vivos
            DD("\nSoy %d, me entere que %d murio \n", me, origen/2);
            int server = origen/2;
            servers_vivos[server] = false;
            cant_servers_vivos--;

            // si estoy pidiendo el recurso, y el server que se está despidiendo todavia no me hizo
            // un reply al pedido, lo marco como que respondio y bajo el reply count.
            // si llego a 0, otorgo el recurso al cliente.
            // Esta solucion es la que sugiere el paper.
            if(hay_pedido_local && respondieron[server] == false){
                outstanding_reply_count--;
                respondieron[server] = true;
                
                if (outstanding_reply_count == 0){
                    debug("Dándole permiso (frutesco por ahora?)");
                    DD("Soy %d, otorgo recurso... ME VOY\n", me);
                    MPI_Send(NULL, 0, MPI_INT, mi_cliente, TAG_OTORGADO, COMM_WORLD);
                }

            }

            // hago lo mismo si me estoy despidiendo, y esperando un CHAU
            // si estoy esperando un CHAU de ese server, marco como que me llego, y disminuyo
            // la cantidad que me falta esperar
            if (chau_lista[server] == false){
                se_despidieron--;
                chau_lista[server] = true;
            }

            // si llega a 0, listo, no tengo que esperar a nadie mas y puedo salir
            if(se_despidieron == 0)
                listo_para_salir = true;

            // respondo con un CHAU
            MPI_Send(NULL, 0, MPI_INT, origen, TAG_CHAU, COMM_WORLD);
        }

        else if (tag == TAG_CHAU) {
            // si me llega un chau, marco que me llego uno de ese servidor (en caso de 
            // que no lo haya marcado ya, por ejemplo si el server murio) 
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

            // me marco como server muerto
            // tengo que esperar cant_servers_vivos - 1 (todos los vivos menos yo) cantidad de CHAUs
            servers_vivos[me] = false;
            se_despidieron = cant_servers_vivos - 1;

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
