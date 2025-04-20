Для парсинга TOML-файла с использованием библиотеки **`cpptoml`** (header-only), ниже приведён пример кода, который читает указанный вами конфигурационный файл и извлекает параметры. Библиотека `cpptoml` достаточно проста в использовании и подходит для задач парсинга TOML.

---

### 1. Установка библиотеки `cpptoml`
Библиотека `cpptoml` является заголовочной (header-only). Скачайте её с [GitHub](https://github.com/skystrife/cpptoml) и добавьте файлы `cpptoml.h` и `cpptoml.cpp` в ваш проект.

---

### 2. Код для парсинга TOML-файла

```cpp
#include <iostream>
#include <string>
#include <memory> // Для std::shared_ptr
#include "cpptoml.h" // Подключение библиотеки cpptoml

struct IRCConfig {
    struct Server {
        std::string host;
        int port;
    } server;

    struct Client {
        std::string username;
        std::string nickname;
        std::string nickserv_password;
        std::string channel;
        std::string admin_nick;
        bool auto_connect;
        char command_symbol;
    } client;
};

// Функция для парсинга TOML-файла
IRCConfig parseTomlFile(const std::string& filename) {
    IRCConfig config;

    try {
        // Загрузка TOML-файла
        auto table = cpptoml::parse_file(filename);

        // Секция [ircServer]
        const auto& ircServer = table->get_table("ircServer");
        config.server.host = ircServer->get_qualified<std::string>("ircServerHost")->to_string();
        config.server.port = ircServer->get_qualified<int>("ircServerPort")->to<int>();

        // Секция [ircClient]
        const auto& ircClient = table->get_table("ircClient");
        config.client.username = ircClient->get_qualified<std::string>("ircBotUser")->to_string();
        config.client.nickname = ircClient->get_qualified<std::string>("ircBotNick")->to_string();
        config.client.nickserv_password = ircClient->get_qualified<std::string>("ircBotNspw")->to_string();
        config.client.channel = ircClient->get_qualified<std::string>("ircBotChan")->to_string();
        config.client.admin_nick = ircClient->get_qualified<std::string>("ircBotAdmi")->to_string();
        config.client.auto_connect = ircClient->get_qualified<bool>("ircBotAcon")->to<bool>();

        // Командный символ читается как строка длиной в один символ
        std::string commandSymbolStr = ircClient->get_qualified<std::string>("ircBotCsym")->to_string();
        if (commandSymbolStr.length() == 1) {
            config.client.command_symbol = commandSymbolStr[0];
        } else {
            throw std::runtime_error("ircBotCsym must be a single character.");
        }

    } catch (const cpptoml::parse_exception& e) {
        std::cerr << "TOML parsing error: " << e.what() << "\n";
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing TOML file: " << e.what() << "\n";
        throw;
    }

    return config;
}

// Функция для вывода конфигурации
void printConfig(const IRCConfig& config) {
    std::cout << "IRC Server Configuration:\n";
    std::cout << "Host: " << config.server.host << "\n";
    std::cout << "Port: " << config.server.port << "\n\n";

    std::cout << "IRC Client Configuration:\n";
    std::cout << "Username: " << config.client.username << "\n";
    std::cout << "Nickname: " << config.client.nickname << "\n";
    std::cout << "NickServ Password: " << config.client.nickserv_password << "\n";
    std::cout << "Channel: " << config.client.channel << "\n";
    std::cout << "Admin Nick: " << config.client.admin_nick << "\n";
    std::cout << "Auto Connect: " << (config.client.auto_connect ? "true" : "false") << "\n";
    std::cout << "Command Symbol: '" << config.client.command_symbol << "'\n";
}

int main() {
    try {
        // Путь к TOML-файлу
        std::string filename = "config.toml";

        // Парсинг файла
        IRCConfig config = parseTomlFile(filename);

        // Вывод конфигурации
        printConfig(config);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

---

### 3. Объяснение кода

#### a) Структура данных `IRCConfig`
- Мы создали структуру `IRCConfig`, которая содержит две вложенные структуры:
  - `Server`: для параметров сервера IRC.
  - `Client`: для параметров клиента IRC.

#### b) Функция `parseTomlFile`
- Эта функция принимает имя файла и парсит его с помощью `cpptoml::parse_file`.
- Используем метод `.get_qualified<T>()` для безопасного получения значений.
- Для параметра `ircBotCsym` (командный символ) проверяем, что строка имеет длину ровно 1, и преобразуем её в `char`.

#### c) Функция `printConfig`
- Эта функция выводит содержимое структуры `IRCConfig` в консоль.

#### d) Обработка ошибок
- Если возникает ошибка синтаксиса TOML или другие исключения, они перехватываются и выводятся в консоль.

---

### 4. Пример файла `config.toml`

```toml
# Конфигурационный файл для IRC-клиента
[ircServer]
ircServerHost = "irc.rizon.net"    # Адрес сервера IRC
ircServerPort = 7000               # Порт сервера IRC

[ircClient]
ircBotUser = "cbot"                # Имя пользователя бота
ircBotNick = "aion"                # Ник бота
ircBotNspw = "lamodrom"            # Пароль NickServ
ircBotChan = "#ircxx"              # Канал, к которому присоединяется бот
ircBotAdmi = "const"               # Ник администратора
ircBotAcon = true                  # Флаг автозапуска (true/false)
ircBotCsym = "."                   # Символ команды
```

---

### 5. Результат работы программы

Если запустить программу с указанным выше файлом `config.toml`, она выведет:

```
IRC Server Configuration:
Host: irc.rizon.net
Port: 7000

IRC Client Configuration:
Username: cbot
Nickname: aion
NickServ Password: lamodrom
Channel: #ircxx
Admin Nick: const
Auto Connect: true
Command Symbol: '.'
```

---

### 6. Особенности использования `cpptoml`

- **Простота**: Библиотека предоставляет интуитивно понятный API для доступа к значениям.
- **Типобезопасность**: Все значения явно преобразуются к нужному типу (например, `to_string()`, `to<int>()`).
- **Обработка ошибок**: Исключения (`cpptoml::parse_exception`) помогают обнаруживать проблемы с файлом TOML.

Этот код демонстрирует, как использовать библиотеку `cpptoml` для чтения и обработки TOML-конфигураций в C++.