
#include <cstring>
#include <fcntl.h>
#include "socket.h"

#define MAXDATASIZE 2048

bool IRCSocket::Init()
{
    if ((_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        std::cout << "Socket error." << std::endl;
        return false;
    }

    int on = 1;
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char const*)&on, sizeof(on)) == -1)
    {
        std::cout << "Invalid socket." << std::endl;
        return false;
    }

    fcntl(_socket, F_SETFL, O_NONBLOCK);
    fcntl(_socket, F_SETFL, O_ASYNC);

    return true;
}

bool IRCSocket::Connect(const char* host, int port)
{
    struct addrinfo hints;
    struct addrinfo* result = nullptr;

    // Заполняем структуру hints для указания параметров разрешения имени хоста
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // Поддержка как IPv4, так и IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP сокет
    hints.ai_protocol = IPPROTO_TCP; // Протокол TCP

    // Преобразуем порт в строку
    std::string portStr = std::to_string(port);

    // Вызываем getaddrinfo для разрешения имени хоста
    int status = getaddrinfo(host, portStr.c_str(), &hints, &result);
    if (status != 0)
    {
        std::cout << "Could not resolve host: " << host << " (" << gai_strerror(status) << ")" << std::endl;
        return false;
    }

    // Перебираем результаты и пытаемся установить соединение
    bool connected = false;
    for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        // Пытаемся установить соединение
        if (connect(_socket, ptr->ai_addr, ptr->ai_addrlen) == SOCKET_ERROR)
        {
            // Если не удалось, продолжаем с следующим адресом
            continue;
        }
        
        // Соединение успешно установлено
        connected = true;
        break;
    }

    // Освобождаем память, выделенную getaddrinfo
    freeaddrinfo(result);

    if (!connected)
    {
        std::cout << "Could not connect to: " << host << std::endl;
        closesocket(_socket);
        return false;
    }

    _connected = true;

    return true;
}

void IRCSocket::Disconnect()
{
    if (_connected)
    {
        closesocket(_socket);
        _connected = false;
    }
}

bool IRCSocket::SendData(char const* data)
{
    if (_connected)
        if (send(_socket, data, strlen(data), 0) == -1)
            return false;

    return true;
}

std::string IRCSocket::ReceiveData()
{
    char buffer[MAXDATASIZE];

    memset(buffer, 0, MAXDATASIZE);

    int bytes = recv(_socket, buffer, MAXDATASIZE - 1, 0);

    if (bytes > 0)
        return std::string(buffer);
    else
        Disconnect();

    return "";
}
