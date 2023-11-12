/**
 * @file
 * @brief Proceso servidor.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief Cliente conectado.
 */
typedef struct {
	pthread_t thread; /*!< ID de hilo (biblioteca) */
	int socket; /*!< Socket del cliente  - accept */
	int id; /*!< Posicion del cliente en el arreglo de clientes*/
	char username[50]; /*!< Nombre de usuario del cliente */
} client;

/** 
 * @brief Adiciona un nuevo cliente al arreglo de clientes.
 * @param Socket del cliente.
 * @return Posicion del cliente en el arreglo, -1 si no se adicion.
 */
int register_client(int c);

/**
 * @brief Elimina un cliente.
 * @param id Identificador del cliente (posicion en el arreglo de clientes)
 * @return 0 si se elimino correctamente, -1 si ocurrio un error.
 */
int remove_client(int id);

/** 
 * @brief Subrutina manejadora de señal. 
 * @param signum Numero de la señal recibida.
 */
void signal_handler(int signum);

/** 
* @brief Hilo que atiende a un cliente.
* @param arg Cliente (client*)
*/
void * client_handler(void * arg);

/**
* @brief Reenvia los mensajes a todos los clientes conectados.
* @param message Mensaje
*/
void broadcast_message(char *message);

/** 
* @brief Envia mensaje privado a un cliente especifico.
* @param message Mensaje
* @param username Nombre de usuario destinatario
*/
void private_message(char *message, char *username);

/** @brief Indica la terminacion del servidor. */
int finished;

/** @brief Arreglo de clientes */
client * clients;

/** @brief Capacidad del arreglo de clientes */
int maxclients;

/** @brief Cantidad de clientes */
int nclients;

/** @brief Mutex como mecanismo de sincronizacion */
pthread_mutex_t lock; 

int main(int argc, char * argv[]) {

	int s;
	int c;
	int i;

	struct sockaddr_in addr;
	struct sockaddr_in caddr;
	socklen_t clilen;

	struct sigaction act;
	//Inicializar mutex
	pthread_mutex_init(&lock, NULL);
	//Instalar manejadores de las señales SIGINT, SIGTERM
	//Rellenar la estructura con ceros.
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = signal_handler;

	//instalar el manejador para SIGINT
	if (sigaction(SIGINT, &act, NULL) != 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	//instalar el manejador para SIGINT
	if (sigaction(SIGTERM, &act, NULL) != 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}


	//1. Obtener un socket
	//s = socket(..)
	//Obtener un socket para IPv4 conexion de flujo
	s = socket(AF_INET, SOCK_STREAM, 0);

	//2. Asociar el socket a una direccion (remota, IPv4)
	//2.1 Configurar la direccion
	// Llenar la direccion con ceros
	memset(&addr, 0, sizeof(struct sockaddr_in));

	addr.sin_family = AF_INET;
	//TODO el puerto es un argumento de linea de comandos
	addr.sin_port = htons(1234);
	if (inet_aton("0.0.0.0", &addr.sin_addr) == 0) {
		perror("inet_aton");
		close(s);
		exit(EXIT_FAILURE);
	}
	int opt = 1;
	//Se verifica que el puerto no haya quedado en estado TIME_WAIT
	//y se configura el socket para que reutilice la direccion y el puerto
	//usando SO_REUSEADDR
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	//2.2 Asociar el socket a la direccion
	if (bind(s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != 0) {
		perror("bind");
		close(s);
		exit(EXIT_FAILURE);
	}

	//3. Marcar el socket como disponible
	if (listen(s, 10) != 0) {
		perror("listen");
		close(s);
		exit(EXIT_FAILURE);
	}

	//4. Esperar la conexion de un cliente
	//El servidor obtiene una copia del socket conectado al cliente
	printf("Waiting for connections...\n");

	finished = 0;

	maxclients = 0;
	nclients = 0;
	clients = NULL;

	while(!finished) {
		clilen = sizeof(struct sockaddr_in);
		c = accept(s, (struct sockaddr *)&caddr, &clilen);
		if (c < 0) {
			if (errno == EINTR) {
				continue;
			}else{
				finished = 1;
				continue;
			}
		}



		//crear un cliente usando una seccion critica
		if (register_client(c) < 0) {
			fprintf(stderr, "Couldn't register the new client!\n");
			close(c);
			continue;
		}
	}

	printf("Server finished.\n");
	//7. Liberar el socket del servidor
	close(s);

	//Esperar que los hilos terminen
	//liberar recursos!
	for (i = 0; i < maxclients; i++) {
		if (clients[i].socket > 0) {
			close(clients[i].socket);
			}
		}

 	free(clients);  

	exit(EXIT_SUCCESS);
}

void signal_handler(int signum) {
	//printf("Signal %d received!\n", signum);
	finished = 1;
}

int register_client(int socket) {
	//Bloquear antes de acceder a los clientes
	pthread_mutex_lock(&lock);
	int i;
	if (nclients == maxclients) {
		//Crecer el arreglo de clientes
		if (maxclients > 0) {
			maxclients = (maxclients * 3)/2;
		}else {
			maxclients = 10;
		}
		client * new_clients = (client*)malloc(maxclients * sizeof(client));

		//Rellenar todo el arreglo de ceros.
		memset(new_clients, 0, maxclients * sizeof(client));
		for (i = 0; i<maxclients;i++) {
			//Marcar el socket como NO USADO
			new_clients[i].socket = -1;
		}

		if (new_clients == NULL) {
			perror("malloc");
			// Desbloquear el mutex
			pthread_mutex_unlock(&lock); 
			return -1;
		}

		//Si existian clientes antes, copiar los datos al nuevo arreglo
		if (nclients > 0) {
			memcpy(new_clients, clients, nclients * sizeof(client));
		}
		free(clients);
		clients = new_clients;
	}

	
	//Pre: El arreglo clients tiene espacios disponibles.
	//Buscar una posicion disponible y registrar el cliente.
	for (i = 0; i<maxclients; i++) {
		if (clients[i].socket == -1) {
			clients[i].socket = socket;
			clients[i].id = i;
			nclients++; //Incrementar la cantidad de clientes registrados
			pthread_create(&clients[i].thread, 
			NULL, 
			client_handler, 
			(void*)&clients[i]
			);
			// Desbloquear el mutex
			pthread_mutex_unlock(&lock);
			return i;
		}
	}
	// Desbloquear el mutex
	pthread_mutex_unlock(&lock);
	return -1;
}

int remove_client(int id) {
	// Bloquear el mutex
	pthread_mutex_lock(&lock);
	close(clients[id].socket);
	clients[id].socket = -1;
	nclients--;
	char message[1024];
    sprintf(message, "Client %s has disconnected.\n", clients[id].username);
    broadcast_message(message);
	// Desbloquear el mutex
	pthread_mutex_unlock(&lock);
	return 0;
}

void * client_handler(void * arg) {
	client * cl = (client*)arg;
	int id = ((client*)arg)->id;
    char username[50];
	//Leer el nombre de usuario ingresado por el cliente
    read(cl->socket, username, sizeof(username));
	// Comprobar si el nombre de usuario ya está en uso
    for (int i = 0; i < maxclients; i++) {
        if (i != id && clients[i].socket != -1 && strcmp(clients[i].username, username) == 0) {
            // Si el nombre de usuario ya está en uso, envía un mensaje de error al cliente
            char *message = "Error: Username is already in use.\n";
            write(cl->socket, message, strlen(message));
            remove_client(id);
            return NULL;
        }
    }

    // Si el nombre de usuario no está en uso, guarda el nombre de usuario en la estructura del cliente
    strncpy(cl->username, username, sizeof(cl->username));
    char *message = "Info: You have successfully registered in the chat.\n";
    write(cl->socket, message, strlen(message));
	
    // Informar a los demás clientes de la nueva conexión
    char welcome_message[1024];
    sprintf(welcome_message, "Client %s has connected.\n", cl->username);
    broadcast_message(welcome_message);

    printf("Handling connection with client: %s\n", username);
	//sleep(5);
	char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(cl->socket, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            printf("Client %s disconnected\n", username);
            break;
        }

        // Comprobar si el mensaje es un comando
        if (buffer[0] == '/') {
            if (strncmp(buffer, "/exit", 5) == 0) {
                printf("Client %s disconnected\n", username);
                break;
            } else if (strncmp(buffer, "/private", 8) == 0) {
                // Enviar un mensaje privado
                char *username = strtok(buffer + 9, " ");
                char *message = strtok(NULL, "");
                private_message(message, username);
            }
        } else {
            // Reenviar el mensaje a todos los clientes
			char message_with_username[1024];
            sprintf(message_with_username, "%s: %s", username, buffer);
            broadcast_message(message_with_username);
        }
    }
	remove_client(id);
}

void broadcast_message(char *message) {
    for (int i = 0; i < maxclients; i++) {
        if (clients[i].socket != -1) {
            write(clients[i].socket, message, strlen(message));
        }
    }
}

void private_message(char *message, char *username) {
    for (int i = 0; i < maxclients; i++) {
        if (clients[i].socket != -1 && strcmp(clients[i].username, username) == 0) {
            write(clients[i].socket, message, strlen(message));
            break;
        }
    }
}

