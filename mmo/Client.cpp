#include <SDL.h>
#include <string>
#include <sstream>

#include "Client.h"
#include "messageCodes.h"

const int Client::BUFFER_SIZE = 100;
const int Client::SCREEN_WIDTH = 640;
const int Client::SCREEN_HEIGHT = 480;

Client::Client(const Args &args):
_args(args),
_location(std::make_pair(0, 0)),
_loop(true),
_debug(SCREEN_HEIGHT/20),
_socket(&_debug){

    int screenX = _args.contains("left") ?
                  _args.getInt("left") :
                  SDL_WINDOWPOS_UNDEFINED;
    int screenY = _args.contains("top") ?
                  _args.getInt("top") :
                  SDL_WINDOWPOS_UNDEFINED;
    _window = SDL_CreateWindow("Client", screenX, screenY, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!_window)
        return;
    _screen = SDL_GetWindowSurface(_window);

    _image = SDL_LoadBMP("Images/man.bmp");

    // For now, randomize player names
    for (int i = 0; i != 3; ++i)
    _username.push_back('a' + rand() % 26);
    _debug << "Player name: " << _username << Log::endl;

    _debug("Client initialized");

    _defaultFont = TTF_OpenFont("trebuc.ttf", 16);

    // Server details
    sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);

    // Connect to server
    if (connect(_socket.getRaw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0)
        _debug << Color::RED << "Connection error: " << WSAGetLastError() << Log::endl;
    else
        _debug << Color::GREEN << "Connected to server" << Log::endl;

    // Announce player name
    std::ostringstream oss;
    oss << '[' << CL_I_AM << ',' << _username << ']';
    _socket.sendMessage(oss.str());
}

Client::~Client(){
    if (_defaultFont)
        TTF_CloseFont(_defaultFont);
    if (_image)
        SDL_FreeSurface(_image);
    if (_window)
        SDL_DestroyWindow(_window);
}

void Client::checkSocket(){
    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    static timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, 0, 0, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << Color::RED << "Error polling sockets: " << WSAGetLastError() << Log::endl;
        return;
    }
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {
        static char buffer[BUFFER_SIZE+1];
        int charsRead = recv(_socket.getRaw(), buffer, 100, 0);
        if (charsRead != SOCKET_ERROR && charsRead != 0){
            buffer[charsRead] = '\0';
            _debug << "recv: " << std::string(buffer) << "" << Log::endl;
            _messages.push(std::string(buffer));
        }
    }
}

void Client::run(){

    if (!_window || !_image)
        return;

    while (_loop) {
        // Deal with any messages from the server
        if (!_messages.empty()){
            handleMessage(_messages.front());
            _messages.pop();
        }

        // Handle user events
        static SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            std::ostringstream oss;
            switch(e.type) {
            case SDL_QUIT:
                _loop = false;
                break;

            case SDL_KEYDOWN:
                switch(e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    _loop = false;
                    break;

                case SDLK_UP:
                    oss << '[' << CL_MOVE_UP << ']';
                    _socket.sendMessage(oss.str());
                    break;
                case SDLK_DOWN:
                    oss << '[' << CL_MOVE_DOWN << ']';
                    _socket.sendMessage(oss.str());
                    break;
                case SDLK_LEFT:
                    oss << '[' << CL_MOVE_LEFT << ']';
                    _socket.sendMessage(oss.str());
                    break;
                case SDLK_RIGHT:
                    oss << '[' << CL_MOVE_RIGHT << ']';
                    _socket.sendMessage(oss.str());
                    break;
                }
                break;

            default:
                // Unhandled event
                ;
            }
        }

        // Draw
        draw();
        checkSocket();
        SDL_Delay(10);
    }
}

void Client::draw(){
    SDL_FillRect(_screen, 0, Color::GREEN/4);
    SDL_Rect drawLoc;
    drawLoc.x = _location.first;
    drawLoc.y = _location.second;
    SDL_Rect borderRect = drawLoc;
    borderRect.x = borderRect.x - 1;
    borderRect.y = borderRect.y - 1;
    borderRect.w = 22;
    borderRect.h = 42;
    SDL_FillRect(_screen, &borderRect, Color::WHITE);
    SDL_BlitSurface(_image, 0, _screen, &drawLoc);
    for (std::map<std::string, std::pair<int, int> >::iterator it = _otherUserLocations.begin(); it != _otherUserLocations.end(); ++it){
        drawLoc.x = it->second.first;
        drawLoc.y = it->second.second;
        SDL_BlitSurface(_image, 0, _screen, &drawLoc);
        SDL_Color color = {0, 0xff, 0xff};
        SDL_Surface *nameSurface = TTF_RenderText_Solid(_defaultFont, it->first.c_str(), color);
        drawLoc.y -= 20;
        SDL_BlitSurface(nameSurface, 0, _screen, &drawLoc);
        SDL_FreeSurface(nameSurface);
    }
    _debug.draw(_screen);
    SDL_UpdateWindowSurface(_window);
}

void Client::handleMessage(const std::string &msg){
    std::istringstream iss(msg);
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];
    while (iss.peek() == '[') {
        iss >>del >> msgCode >> del;
        switch(msgCode) {

        case SV_LOCATION:
        {
            std::string name;
            int x, y;
            iss.get(buffer, BUFFER_SIZE, ',');
            name = std::string(buffer);
            iss >> del >> x >> del >> y >> del;
            if (del != ']')
                return;
            if (name == _username)
                _location = std::make_pair(x, y);
            else
                _otherUserLocations[name] = std::make_pair(x, y);
            break;
        }

        case SV_USER_DISCONNECTED:
        {
            std::string name;
            iss.get(buffer, BUFFER_SIZE, ']');
            name = std::string(buffer);
            iss >> del;
            if (del != ']')
                return;
            _otherUserLocations.erase(name);
            _debug << name << " disconnected." << Log::endl;
            break;
        }

        default:
            _debug << Color::RED << "Unhandled message: " << msg;
        }
    }
}
