#include <SDL.h>
#include <sstream>

#include "Client.h" //TODO remove; only here for random initial placement
#include "Socket.h"
#include "Server.h"
#include "User.h"
#include "messageCodes.h"

const int Server::MAX_CLIENTS = 20;
const int Server::BUFFER_SIZE = 100;

Server::Server(const Args &args):
_args(args),
_loop(true),
_debug(30),
_socket(&_debug){
    _debug << args << Log::endl;

    int screenX = _args.contains("left") ?
                  _args.getInt("left") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenY = _args.contains("top") ?
                  _args.getInt("top") :
                  SDL_WINDOWPOS_UNDEFINED;
    _window = SDL_CreateWindow("Server", screenX, screenY, 800, 600, SDL_WINDOW_SHOWN);
    if (!_window)
        return;
    _screen = SDL_GetWindowSurface(_window);

    _debug("Server initialized");

    // Socket details
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    _socket.bind(serverAddr);
    _debug << "Server address: " << inet_ntoa(serverAddr.sin_addr) << ":" << ntohs(serverAddr.sin_port) << Log::endl;
    _socket.listen();

    _debug("Listening for connections");
}

Server::~Server(){
    if (_window)
        SDL_DestroyWindow(_window);
}

void Server::checkSockets(){
    // Populate socket list with active sockets
    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    for (std::set<Socket>::iterator it = _clientSockets.begin(); it != _clientSockets.end(); ++it)
        FD_SET(it->getRaw(), &readFDs);

    // Poll for activity
    static timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, 0, 0, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << Color::RED << "Error polling sockets: " << WSAGetLastError() << Log::endl;
        return;
    }

    // Activity on server socket: new connection
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {
        if (_clientSockets.size() == MAX_CLIENTS)
           _debug("No room for additional clients; all slots full");
        else {
            sockaddr_in clientAddr;
            SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
            if (tempSocket == SOCKET_ERROR) {
                _debug << Color::RED << "Error accepting connection: " << WSAGetLastError() << Log::endl;
            } else {
                _debug << Color::GREEN << "Connection accepted: "
                       << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port)
                       << ", socket number = " << tempSocket << Log::endl;
                _clientSockets.insert(tempSocket);
            }
        }
    }

    // Activity on client socket: message received or client disconnected
    for (std::set<Socket>::iterator it = _clientSockets.begin(); it != _clientSockets.end();) {
        SOCKET raw = it->getRaw();
        if (FD_ISSET(raw, &readFDs)) {
            sockaddr_in clientAddr;
            getpeername(raw, (sockaddr*)&clientAddr, (int*)&Socket::sockAddrSize);
            static char buffer[BUFFER_SIZE+1];
            int charsRead = recv(raw, buffer, BUFFER_SIZE, 0);
            if (charsRead == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAECONNRESET) {
                    // Client disconnected
                    _debug << "Client " << raw << " disconnected" << Log::endl;
                    removeUser(raw);
                    closesocket(raw);
                    _clientSockets.erase(it++);
                    continue;
                } else {
                    _debug << Color::RED << "Error receiving message: " << err << Log::endl;
                }
            } else if (charsRead == 0) {
                // Client disconnected
                _debug << "Client " << raw << " disconnected" << Log::endl;
                removeUser(*it);
                closesocket(raw);
                _clientSockets.erase(it++);
                continue;
            } else {
                // Message received
                buffer[charsRead] = '\0';
                _debug << "recv from client " << raw << ": " << buffer << Log::endl;
                _messages.push(std::make_pair(*it, std::string(buffer)));
            }
        }
        ++it;
    }
}

void Server::run(){
    while (_loop) {
        // Deal with any messages from the server
        while (!_messages.empty()){
            handleMessage(_messages.front().first, _messages.front().second);
            _messages.pop();
        }

        // Handle user events
        static SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            switch(e.type) {
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_ESCAPE){
                    _loop = false;
                }
                break;
            case SDL_QUIT:
                _loop = false;
                break;

            default:
                // Unhandled event
                ;
            }
        }

        draw();

        checkSockets();

        SDL_Delay(10);
    }
}

void Server::draw() const{
    SDL_FillRect(_screen, 0, Color::BLACK);
    _debug.draw(_screen);
    SDL_UpdateWindowSurface(_window);
}

void Server::addUser(const Socket &socket, const std::string &name){
    std::pair<int, int> location = std::make_pair(rand() % (Client::SCREEN_WIDTH - 20),
                                                  rand() % (Client::SCREEN_HEIGHT - 40));
    User newUser(name, location, socket);

    // Send new user everybody else's location
    for (std::list<User>::const_iterator it = _users.begin(); it != _users.end(); ++it)
        sendCommand(newUser, it->makeLocationCommand());

    // Add new user to list, and broadcast his location
    _users.push_back(newUser);
    broadcast(newUser.makeLocationCommand());
    _debug << "New user, " << name << " has been registered." << Log::endl;
}

void Server::removeUser(const Socket &socket){
    for (std::list<User>::iterator it = _users.begin(); it != _users.end(); ++it)
        if (it->getSocket() == socket){
            // Broadcast message
            std::ostringstream oss;
            oss << '[' << SV_USER_DISCONNECTED << ',' << it->getName() << ']';
            broadcast(oss.str());
            _users.erase(it);
            break;
        }
}

void Server::handleMessage(const Socket &client, const std::string &msg){
    int eof = std::char_traits<wchar_t>::eof();
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];
    bool sendLocation = false;
    std::istringstream iss(msg);
    User *user = 0;
    while (iss.peek() == '[') {
        iss >> del >> msgCode >> del;
        
        // Discard message if this client has not yet sent CL_I_AM
        user = getUserBySocket(client);
        if (!user && msgCode != CL_I_AM)
            continue;

        switch(msgCode) {

        case CL_MOVE_UP:
            if (del != ']')
                return;
            user->location.second -= 20;
            sendLocation = true;
            break;

        case CL_MOVE_DOWN:
            if (del != ']')
                return;
            user->location.second += 20;
            sendLocation = true;
            break;

        case CL_MOVE_LEFT:
            if (del != ']')
                return;
            user->location.first -= 20;
            sendLocation = true;
            break;

        case CL_MOVE_RIGHT:
            if (del != ']')
                return;
            user->location.first += 20;
            sendLocation = true;
            break;

        case CL_I_AM:
        {
            std::string name;
            iss.get(buffer, BUFFER_SIZE, ']');
            name = std::string(buffer);
            iss >> del;
            if (del != ']')
                return;

            // Check that username is valid
            bool invalid = false;
            for (std::string::const_iterator it = name.begin(); it != name.end(); ++it){
                if (*it < 'a' || *it > 'z') {
                    std::ostringstream oss;
                    oss << '[' << SV_INVALID_USERNAME << ']' << Log::endl;
                    _socket.sendMessage(oss.str(), client);
                    invalid = true;
                    break;
                }
            }
            if (invalid)
                break;

            // Check that user isn't already logged in
            for (std::list<User>::const_iterator it = _users.begin(); it != _users.end(); ++it) {
                if (it->getName() == name){
                    std::ostringstream oss;
                    oss << '[' << SV_DUPLICATE_USERNAME << ']' << Log::endl;
                    _socket.sendMessage(oss.str(), client);
                    invalid = true;
                    break;
                }
            }
            if (invalid)
                break;

            addUser(client, name);
            break;
        }

        default:
            _debug << Color::RED << "Unhandled message: " << msg;
        }
    }

    if (user && sendLocation) {
        broadcast(user->makeLocationCommand());
    }

}

void Server::sendCommand(const User &dstUser, const std::string &msg) const{
    _socket.sendMessage(msg, dstUser.getSocket());
}

void Server::broadcast(const std::string &msg) const{
    for (std::list<User>::const_iterator it = _users.begin(); it != _users.end(); ++it)
        sendCommand(*it, msg);
}

User *Server::getUserBySocket(const Socket &socket){
    for (std::list<User>::iterator it = _users.begin(); it != _users.end(); ++it)
        if (it->getSocket() == socket)
            return &*it;
    return 0;
}
