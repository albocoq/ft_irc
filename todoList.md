## 🌐 Persona A: Arquitecto de Red (Sockets y Multiplexación)

**Objetivo:** Construir el puente de comunicación entre Internet y el Motor Core. No tocarás la lógica de los comandos IRC, solo el flujo de bytes.

* **Apertura del Servidor:** Crear el "Socket" principal, vincularlo a un puerto de red (`bind`) y ponerlo a escuchar conexiones entrantes (`listen`).
* **Multiplexación (El corazón de la red):** Implementar `epoll`, `poll` o `select`. El objetivo es tener un solo bucle capaz de vigilar decenas de conexiones simultáneamente sin bloquear el programa.
* **I/O No Bloqueante:** Configurar todos los File Descriptors (FDs) con la opción `O_NONBLOCK` usando la función `fcntl` (Requisito estricto de 42).
* **Recepción (Read):** Cuando el multiplexor detecte actividad entrante, usar `recv()` para capturar el texto en bruto e inyectarlo en el motor usando: `cliente->appendReadBuffer(datos)`.
* **Envío (Write):** Comprobar constantemente si el cliente tiene respuestas pendientes. Si `cliente->getWriteBuffer()` no está vacío, usar `send()` para enviarlo por red y luego vaciar el búfer.
* **Gestión de Desconexión:** Vigilar el método `cliente->isToBeDisconnected()`. Si devuelve `true` (ej. tras un comando `QUIT`), cerrar la conexión de red con `close(fd)` y destruir el objeto de forma segura.


Te falta:

🔁 Bucle principal con poll()
🔌 Aceptar nuevos clientes
📥 Leer datos (recv)
📤 Enviar datos (send)
❌ Desconectar clientes