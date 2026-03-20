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

## 📡 Guía de Integración (Para la Persona 1: Red y Sockets)

Tu objetivo es hacer de "puente" entre Internet (epoll/poll) y este Motor Core. Este es el flujo exacto que debes implementar en tu bucle principal:

1. **Lectura de Red:** Cuando tu multiplexor (epoll/poll) detecte datos en un FD, léelos con `recv()` y envíalos al cliente:
   `cliente->appendReadBuffer(datos_recibidos);`
2. **Procesamiento:** Inicia un bucle `while` para extraer todas las líneas completas posibles:
   `std::string linea = cliente->extractLine();`
3. **Ejecución:** Por cada línea extraída, crea el mensaje y pásalo al enrutador junto con la lista global de clientes:
   `Message msg(linea);`
   `handler.execute(*cliente, msg, vector_global_clientes);`
4. **Escritura en Red:** Revisa si el motor ha generado respuestas. Si es así, envíalas por el socket y limpia el búfer:
   `if (!cliente->getWriteBuffer().empty()) { send(...); }`
5. **Desconexión:** Al final del ciclo, comprueba si el motor ha marcado al cliente para ser expulsado:
   `if (cliente->isToBeDisconnected()) { close(fd); /* eliminar de la lista */ }`

---

## 💬 Guía de Expansión (Para la Persona 3: Canales)

Tu objetivo es añadir la lógica de salas de chat y moderación utilizando la base que ya existe en el `CommandHandler`.

1. **Nueva Clase `Channel`:** Crea una clase para representar una sala. Debe contener:
   * Lista de miembros (punteros a `Client`).
   * Lista de operadores.
   * El `Topic` (tema) de la sala.
   * Los modos de la sala (contraseña, límite de usuarios, etc.).
2. **Nuevos Comandos:** Abre `CommandHandler.cpp` y añade las siguientes funciones al mapa de comandos:
   * `JOIN`: Unirse o crear un canal.
   * `PART`: Salir de un canal.
   * `KICK`: Expulsar a un usuario (requiere verificar si el emisor es operador).
   * `TOPIC`: Ver o modificar el tema.
   * `MODE`: Modificar las reglas del canal.
3. **Actualizar `PRIVMSG`:** En la función actual de `handlePrivmsg`, busca el comentario `// TODO: Trabajo de persona 3` dentro de la condición `if (recipient[0] == '#')`. Allí deberás iterar sobre los miembros del canal objetivo y enviarles el mensaje a todos (excepto al remitente).

---
*Fin del documento.*