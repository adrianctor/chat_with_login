Elaborado por: Adrian Camilo Torres Gomez <adriantor@unicauca.edu.co>

Laboratorio de comunicacion entre procesos.
Este laboratorio muestra el esquema basico de comunicacion
remota mediante sockets IPv4.

- server.c : Proceso servidor.
- client.c: Proceso cliente.

Usabilidad:
Comandos para el cliente:
- /exit: Cierra la conexion entre el servidor y ese cliente.
- /private username message: Envia un mensaje (message) a un usuario en especifico (username)

Descripcion:
1.1. Funcionamiento del cliente
El cliente deberá solicitar por entrada estándar (o como argumento de línea
de comandos) el login (nombre de usuario) con el cual se registrará en el chat.

Cuando un cliente se conecte al servidor de manera exitosa, el cliente de-
berá enviar un mensaje al servidor, informando el nombre de usuario deseado.

El servidor deberá responder con un mensaje de confirmación, o de error si el
nombre de usuario está siendo usado.
Posteriormente, el cliente tendrá que ejecutar dos tareas (usando hilos):
una tarea recibirá líneas de texto por entrada estándar, y las enviará al servidor,
quien las replicará a los demás clientes conectados. La otra tarea del cliente
deberá esperar por mensajes enviados por el servidor, y los mostrará por la
salida estándar (la pantalla).
Tanto el servidor como el cliente deberán terminar cuando se reciba la señal
SIGTERM. Se puede agregar otros manejadores de señales, como SIGINT o
SIGQUIT.
1.2. Comandos del cliente

El cliente recibe líneas de texto de la entrada estándar y las envía al servi-
dor. Si una línea comienza con el caracter /"se considerará un comando. Los

comandos a implementar son:

/exit: Envía este mensaje al servidor y cierra la conexión. Es necesario en-
viar el mensaje, ya que el servidor debe informar a los demás clientes que se

ha cerrado una conexión.
/private login text: Envía un mensaje privado a otro usuario conectado.

1.3. Funcionamiento del servidor
El proceso servidor deberá recibir una conexión del cliente, verificar que el
nombre de usuario especificado no se encuentre usado en el momento y crear
una nueva tarea (hilo) que atenderá esta conexión.
El hilo se encargará de recibir mensajes del cliente conectado y reenviarlo
a los demás clientes que también se encuentren conectados para los mensajes
públicos, o sólo al usuario seleccionado si se envía un mensaje privado.
Cuando un cliente envía el comando /exit al servidor, se deberá enviar un
mensaje a los demás clientes conectados.
Debido a que cada cliente se gestiona mediante un hilo, se deberá tener
una estructura de datos central que permita llevar la cuenta de los clientes
conectados (los descriptores obtenidos en la llamada accept). Esta estructura
de datos es un recurso compartido que deberá ser protegido mediante algún
mecanismo de sincronización.
El servidor repetirá el proceso de recibir y atender solicitudes hasta que se
reciba la señal SIGTERM. En este momento deberá informar a todos los clientes
conectados y cerrar todas las conexiones.