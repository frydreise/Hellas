#include <SDL.h>
#include <string>
#include <sstream>

#include "Client.h"
#include "messageCodes.h"

const int Client::BUFFER_SIZE = 100;

int startSocketClient(void *client){
    ((Client*)client)->runSocketClient();
    return 0;
}

Client::Client():
_location(std::make_pair(0, 0)),
_loop(true){
    SDL_Thread *socketThreadID = SDL_CreateThread(startSocketClient, "Client socket handler", this);

    int ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret < 0)
        return;

    _window = SDL_CreateWindow("Client", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!_window)
        return;
    _screen = SDL_GetWindowSurface(_window);

    _image = SDL_LoadBMP("Images/man.bmp");
}

Client::~Client(){
    if (_image)
        SDL_FreeSurface(_image);
    if (_window)
        SDL_DestroyWindow(_window);
    SDL_Quit();
}

void Client::runSocketClient(){
    // Server details
    sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);

    // Connect to server
    if (connect(_socket.raw(), (sockaddr*)&serverAddr, Socket::sockAddrSize) < 0){
        std::cout << "Connection error" << std::endl;
        return ;
    }
    std::cout << "Connected" << std::endl;

    //Receive messages indefinitely
    char buffer[BUFFER_SIZE+1];
    for (int i = 0; i != BUFFER_SIZE; ++i)
        buffer[i] = '\0';
    while (true) {
        int charsRead = recv(_socket.raw(), buffer, 100, 0);
        if (charsRead != SOCKET_ERROR){
            buffer[charsRead] = '\0';
            std::cout << "Received message: \"" << std::string(buffer) << "\"" << std::endl;
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
                case SDLK_UP:
                    oss << '[' << REQ_MOVE_UP << ']';
                    _socket.sendCommand(oss.str());
                    break;
                case SDLK_DOWN:
                    oss << '[' << REQ_MOVE_DOWN << ']';
                    _socket.sendCommand(oss.str());
                    break;
                case SDLK_LEFT:
                    oss << '[' << REQ_MOVE_LEFT << ']';
                    _socket.sendCommand(oss.str());
                    break;
                case SDLK_RIGHT:
                    oss << '[' << REQ_MOVE_RIGHT << ']';
                    _socket.sendCommand(oss.str());
                    break;
                }
                break;

            default:
                //Unhandled event
                ;
            }
        }

        // Draw
        draw();
    }
}

void Client::draw(){
    SDL_FillRect(_screen, 0, SDL_MapRGB(_screen->format, 0, 96, 0));
    SDL_Rect drawLoc;
    drawLoc.x = _location.first;
    drawLoc.y = _location.second;
    SDL_Rect borderRect = drawLoc;
    borderRect.x = borderRect.x - 1;
    borderRect.y = borderRect.y - 1;
    borderRect.w = 22;
    borderRect.h = 42;
    SDL_FillRect(_screen, &borderRect, SDL_MapRGB(_screen->format, 255, 255, 255));
    SDL_BlitSurface(_image, 0, _screen, &drawLoc);
    for (std::map<SOCKET, std::pair<int, int> >::iterator it = _otherUserLocations.begin(); it != _otherUserLocations.end(); ++it){
        drawLoc.x = it->second.first;
        drawLoc.y = it->second.second;
        SDL_BlitSurface(_image, 0, _screen, &drawLoc);
    }
    SDL_UpdateWindowSurface(_window);
}

void Client::handleMessage(std::string msg){
    std::istringstream iss(msg);
    int msgCode;
    char del;
    while (iss.peek() == '[') {
        iss >>del >> msgCode >> del;
        switch(msgCode) {

        case MSG_LOCATION:
        {
            int x, y;
            iss >> x >> del >> y >> del;
            if (del != ']')
                return;
            _location = std::make_pair(x, y);
        }

        case MSG_OTHER_LOCATION:
        {
            int s, x, y;
            iss >> s >> del >> x >> del >> y >> del;
            if (del != ']')
                return;
            _otherUserLocations[(SOCKET)s] = std::make_pair(x, y);
        }

        default:
            ;
        }
    }
}
