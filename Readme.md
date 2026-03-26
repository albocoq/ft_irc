# ⚙️ ft_irc - Motor Core y Arquitectura 

> **Estado del Módulo:** ✅ Completado (Motor Base)
> **Responsable:** Persona 2
> **Estándar:** C++98 Compliant

Este documento detalla la arquitectura central del servidor IRC. El núcleo de procesamiento de texto, enrutamiento de comandos y gestión de estado de los clientes ya está construido y listo para ser integrado con la capa de Red (Sockets) y la capa de Salas (Canales).

---

## 🏗️ Arquitectura Actual (Lo que ya está construido)

El núcleo se divide en tres clases principales que garantizan la seguridad de la memoria y el cumplimiento del protocolo IRC:

### 1. `Client` (Gestión de Estado y Memoria)
Representa a un usuario individual conectado al servidor.
* **Buffers Inteligentes:** Contiene un `_readBuffer` y un `_writeBuffer`.
* **Extracción Segura:** La función `extractLine()` recorta frases perfectas delimitadas por `\r\n`, evitando bloqueos si los datos de la red llegan fragmentados.
* **Seguimiento de Estado (Handshake):** Banderas booleanas que indican si el usuario ha validado `PASS`, `NICK` y `USER`.
* **Desconexión Segura:** Implementa el flag `_toDisconnected` para avisar al servidor de red cuándo es seguro cerrar el socket (evitando *segfaults*).

### 2. `Message` (El Parseador IRC)
El analista gramatical del servidor. Toma una cadena de texto en bruto y la divide siguiendo las estrictas reglas del protocolo RFC de IRC.
* Extrae el **Prefijo**, el **Comando** y los **Parámetros**.
* Maneja perfectamente la regla del *Trailing Parameter* (`:`), permitiendo procesar mensajes con espacios continuos de forma segura.

### 3. `CommandHandler` (El Enrutador / Dispatcher)
El cerebro de las operaciones. Utiliza un `std::map` de punteros a funciones miembro para ejecutar comandos en tiempo constante ($O(1)$).
* **Autenticación:** Procesa `PASS`, `NICK` (con sistema anti-clones para evitar apodos duplicados) y `USER`. Automáticamente envía el `001 RPL_WELCOME` al completar el registro.
* **Mensajería Base:** Implementa `PRIVMSG` para mensajes privados de usuario a usuario.
* **Mantenimiento:** Responde a `PING` con `PONG` (evitando el *timeout* del cliente) y gestiona el cierre con `QUIT`.
* **Errores Oficiales:** Responde con los códigos numéricos exactos del estándar IRC (ej. `461`, `464`, `433`, `401`).

---

## 🌐 Persona A: Arquitecto de Red (Sockets y Multiplexación)

**Objetivo:** Construir el puente de comunicación entre Internet y el Motor Core. No tocarás la lógica de los comandos IRC, solo el flujo de bytes.

* **Apertura del Servidor:** Crear el "Socket" principal, vincularlo a un puerto de red (`bind`) y ponerlo a escuchar conexiones entrantes (`listen`).
* **Multiplexación (El corazón de la red):** Implementar `epoll`, `poll` o `select`. El objetivo es tener un solo bucle capaz de vigilar decenas de conexiones simultáneamente sin bloquear el programa.
* **I/O No Bloqueante:** Configurar todos los File Descriptors (FDs) con la opción `O_NONBLOCK` usando la función `fcntl` (Requisito estricto de 42).
* **Recepción (Read):** Cuando el multiplexor detecte actividad entrante, usar `recv()` para capturar el texto en bruto e inyectarlo en el motor usando: `cliente->appendReadBuffer(datos)`.
* **Envío (Write):** Comprobar constantemente si el cliente tiene respuestas pendientes. Si `cliente->getWriteBuffer()` no está vacío, usar `send()` para enviarlo por red y luego vaciar el búfer.
* **Gestión de Desconexión:** Vigilar el método `cliente->isToBeDisconnected()`. Si devuelve `true` (ej. tras un comando `QUIT`), cerrar la conexión de red con `close(fd)` y destruir el objeto de forma segura.

---

## 🏠 Persona B: Arquitecto de Salas (Gestión de Canales)

**Objetivo:** Apoyarse en el enrutador actual (`CommandHandler`) para crear y gestionar el concepto de "Salas de chat" (canales que empiezan por `#`).

* **La Clase `Channel`:** Crear este nuevo objeto. Debe contener el nombre de la sala y una lista de los usuarios que están dentro (ej. un `std::vector<Client*>`).
* **Directorio de Salas:** Añadir un `std::map` global en el servidor o en el enrutador para almacenar y buscar rápidamente las salas activas.
* **Comando `JOIN`:** * Si la sala no existe, crearla y añadir al usuario. 
  * Si existe, añadir al usuario. 
  * Enviar mensaje a los presentes avisando de la entrada y responder al nuevo usuario con la lista de miembros (`353 RPL_NAMREPLY`).
* **Comando `PART`:** Retirar al usuario de la lista de la sala. Si la sala se queda completamente vacía, destruir el objeto `Channel` para liberar memoria.
* **Actualizar `PRIVMSG`:** Modificar la función `handlePrivmsg` existente. Cuando el objetivo empiece por `#`, buscar la sala e iterar para copiar el mensaje en el `_writeBuffer` de **todos** los miembros (excepto el remitente).

---

## 🛡️ Persona C: Moderador (Privilegios y Comandos Avanzados)

**Objetivo:** Implementar las reglas de administración y los poderes especiales dentro de las salas creadas por la Persona B.

* **Estado de Operador:** Actualizar la clase `Channel` para diferenciar a un usuario normal de un operador (usualmente indicado con un `@` en su apodo).
* **Comando `KICK`:** Función para expulsar a un usuario de una sala. Requiere verificar primero si el emisor tiene privilegios de operador en esa sala específica.
* **Comando `TOPIC`:** Permitir a los usuarios consultar el tema actual de la sala, o modificarlo si tienen permiso.
* **Comando `INVITE`:** Permitir invitar a un usuario específico (que está en el servidor pero no en la sala) a unirse a un canal privado.
* **Comando `MODE`:** El comando más complejo. Permite cambiar la configuración de la sala. Se deben gestionar 5 modos:
  * `+i`: Establecer la sala como "Solo por invitación".
  * `+t`: Restringir el derecho de cambiar el `TOPIC` solo a los operadores.
  * `+k`: Añadir una contraseña para poder entrar a la sala.
  * `+o`: Dar (o quitar) privilegios de operador a otro miembro.
  * `+l`: Definir un límite máximo de usuarios simultáneos en la sala.

---
*Fin del documento.*