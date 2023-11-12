/**
* @file
* @brief Proceso cliente.
*/
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/** 
* @brief Envia mensajes al servidor.
* @param arg Mensaje
*/
void * send_messages(void *arg);

/** 
* @brief Recibe mensajes del servidor.
* @param arg Mensaje
*/
void * receive_messages(void *arg);

int main(int argc, char * argv[]) {

	int s;
	struct sockaddr_in addr;

	char username[50];

    printf("Please enter your username: ");
    fgets(username, sizeof(username), stdin);
    // Eliminar el salto de línea al final del nombre de usuario
    username[strcspn(username, "\n")] = '\0';

	//1. Obtener un conector
	s = socket(AF_INET, SOCK_STREAM, 0);

	//2. Conectarse al servidor
	// 2.1 Configurar la direccion a la cual se va a conectar
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);
	if (inet_aton("127.0.0.1", &addr.sin_addr) == 0) {
		perror("inet_aton");
		close(s);
		exit(EXIT_FAILURE);
	}


	// 2.2 Conectarse al servidor
	// connect(s, ...
	printf("Connecting to server...\n");
	if (connect(s, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) != 0) {
		perror("connect");
		close(s);
		exit(EXIT_FAILURE);
		}


	//3. Comunicacion (write/read)
	printf("Connected\n");
	//sleep(5);
	
	write(s, username, strlen(username));
	pthread_t send_thread, receive_thread;

    // Crear un hilo para enviar mensajes
    pthread_create(&send_thread, NULL, send_messages, &s);

    // Crear un hilo para recibir mensajes
    pthread_create(&receive_thread, NULL, receive_messages, &s);

    // Esperar a que los hilos terminen
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

	printf("Closing connection..\n");

	//4. Terminar la comunicacion
	close(s);

	exit(EXIT_SUCCESS);
}

void * send_messages(void *arg) {
    int s = *(int *)arg;
    char buffer[1024];
    while (1) {
		//printf(">> ");
        fgets(buffer, sizeof(buffer), stdin);
        write(s, buffer, strlen(buffer));
    }
}

void * receive_messages(void *arg) {
    int s = *(int *)arg;
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(s, buffer, sizeof(buffer) - 1);
        if (n <= 0) {
            printf("Disconnected from server\n");
            exit(0);
        }
        printf("%s", buffer);
    }
}