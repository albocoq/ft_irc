📡 ✅ TODO LIST REAL (Persona 1 — Red & Sockets)

Esto está basado EXACTAMENTE en:

tu clase Client
tu CommandHandler
tu Message
y el flujo del README
🧱 1. Crear clase Server (o equivalente)

👉 Debe contener:

int _serverFd
std::vector<pollfd> _fds
std::vector<Client*> _clients
CommandHandler _handler
🔌 2. Inicialización del servidor
✔ Crear socket
socket(AF_INET, SOCK_STREAM, 0);
✔ Configurar
setsockopt(... SO_REUSEADDR ...)
✔ Non-blocking
fcntl(fd, F_SETFL, O_NONBLOCK);
✔ Bind + listen
📊 3. Inicializar poll()

👉 Añadir SOLO el server_fd al principio:

pollfd p;
p.fd = server_fd;
p.events = POLLIN;
fds.push_back(p);
🔁 4. Loop principal (EL CORAZÓN)
while (true)
{
    poll(...);

    for (cada fd)
    {
        if (server_fd)
            acceptNewClient();

        else
            handleClient(fd);
    }
}
👥 5. acceptNewClient()
✔ accept()
✔ non-blocking
✔ crear Client
Client* newClient = new Client(fd, ip);
clients.push_back(newClient);
✔ añadir a poll
📥 6. handleClient() → RECEPCIÓN
✔ recv()
char buffer[1024];
int bytes = recv(fd, buffer, sizeof(buffer), 0);
🚨 CASOS IMPORTANTES
❌ bytes == 0 → cliente desconectado
client->setToBeDisconnected(true);
❌ bytes < 0 → error (ignorar o cerrar)
✔ añadir al buffer (USANDO TU CORE)
client->appendReadBuffer(std::string(buffer, bytes));

💥 ESTO YA LO HACES PERFECTO

🔄 7. PROCESAR COMANDOS (CLAVE)

👉 Aquí usas directamente tu diseño:

while (true)
{
    std::string line = client->extractLine();

    if (line.empty())
        break;

    Message msg(line);
    handler.execute(*client, msg, clients);
}

💥 Esto es EXACTAMENTE lo que tu README describe
💥 y está perfecto con tu extractLine()

📤 8. ENVÍO DE RESPUESTAS (MUY IMPORTANTE)

Tu core escribe en:

client->_writeBuffer

👉 Persona 1 debe hacer:

std::string& out = client->getWriteBuffer();

if (!out.empty())
{
    send(fd, out.c_str(), out.size(), 0);
    client->setWriteBuffer("");
}
🚨 ERROR QUE TIENES AHORA (importante)

Tu getWriteBuffer() devuelve copia ❌

std::string getWriteBuffer() const;

👉 Persona 1 NO podrá vaciar bien el buffer

✔ SOLUCIÓN

Cambiar a:

std::string& getWriteBuffer();
❌ 9. DESCONEXIÓN LIMPIA

Después del loop:

if (client->isToBeDisconnected())
{
    close(fd);
    eliminar de poll
    eliminar de clients
    delete client;
}
🧠 10. Gestión de múltiples clientes

Tu CommandHandler YA usa:

std::vector<Client*>& annular

👉 Persona 1 SOLO debe pasar:

handler.execute(*client, msg, clients);

💥 Esto ya conecta TODO el sistema

⚠️ 11. CONTROL DE ERRORES (OBLIGATORIO 42)

✔ poll falla
✔ recv falla
✔ send falla
✔ accept falla

👉 Nunca crash

🔐 12. RESPETAR HANDSHAKE (ya hecho por Persona 2)

Persona 1 NO toca esto, pero debe saber:

PASS
NICK
USER

👉 ya lo controla CommandHandler

🧪 13. TEST REAL (OBLIGATORIO)
nc -C 127.0.0.1 6667

Testear:

comandos partidos → TU buffer lo soporta ✅
varios clientes
QUIT
PRIVMSG