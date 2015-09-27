// (C) 2015 Tim Gurto

#include <SDL.h>
#include <cassert>
#include <sstream>
#include <fstream>
#include <utility>

#include "Client.h" //TODO remove; only here for random initial placement
#include "Object.h"
#include "ObjectType.h"
#include "Renderer.h"
#include "Socket.h"
#include "Server.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

extern Args cmdLineArgs;
extern Renderer renderer;

const int Server::MAX_CLIENTS = 20;
const size_t Server::BUFFER_SIZE = 1023;

const Uint32 Server::CLIENT_TIMEOUT = 10000;
const Uint32 Server::MAX_TIME_BETWEEN_LOCATION_UPDATES = 300;

const Uint32 Server::SAVE_FREQUENCY = 1000;

const double Server::MOVEMENT_SPEED = 80;
const int Server::ACTION_DISTANCE = 20;

bool Server::isServer = false;

const int Server::TILE_W = 32;
const int Server::TILE_H = 32;

Server::Server():
_time(SDL_GetTicks()),
_lastTime(_time),
_socket(),
_loop(true),
_mapX(0),
_mapY(0),
_debug(100, "server.log"),
_lastSave(_time){
    isServer = true;

    _debug << cmdLineArgs << Log::endl;
    Socket::debug = &_debug;

    loadData();

    _debug("Server initialized");

    // Socket details
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    _socket.bind(serverAddr);
    _debug << "Server address: " << inet_ntoa(serverAddr.sin_addr)
           << ":" << ntohs(serverAddr.sin_port) << Log::endl;
    _socket.listen();

    _debug("Listening for connections");
}

Server::~Server(){
    saveData();
}

void Server::checkSockets(){
    // Populate socket list with active sockets
    static fd_set readFDs;
    FD_ZERO(&readFDs);
    FD_SET(_socket.getRaw(), &readFDs);
    for (const Socket &socket : _clientSockets) {
        FD_SET(socket.getRaw(), &readFDs);
    }

    // Poll for activity
    static timeval selectTimeout = {0, 10000};
    int activity = select(0, &readFDs, 0, 0, &selectTimeout);
    if (activity == SOCKET_ERROR) {
        _debug << Color::RED << "Error polling sockets: " << WSAGetLastError() << Log::endl;
        return;
    }
    _time = SDL_GetTicks();

    // Activity on server socket: new connection
    if (FD_ISSET(_socket.getRaw(), &readFDs)) {

        if (_clientSockets.size() == MAX_CLIENTS) {
            _debug("No room for additional clients; all slots full");
            sockaddr_in clientAddr;
            SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr*)&clientAddr,
                                       (int*)&Socket::sockAddrSize);
            Socket s(tempSocket);
            // Allow time for rejection message to be sent before closing socket
            s.delayClosing(5000);
            sendMessage(s, SV_SERVER_FULL);
        } else {
            sockaddr_in clientAddr;
            SOCKET tempSocket = accept(_socket.getRaw(), (sockaddr*)&clientAddr,
                                       (int*)&Socket::sockAddrSize);
            if (tempSocket == SOCKET_ERROR) {
                _debug << Color::RED << "Error accepting connection: "
                       << WSAGetLastError() << Log::endl;
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
                _messages.push(std::make_pair(*it, std::string(buffer)));
            }
        }
        ++it;
    }
}

void Server::run(){
    while (_loop) {
        _time = SDL_GetTicks();
        const Uint32 timeElapsed = _time - _lastTime;
        _lastTime = _time;

        // Check that clients are alive
        for (std::set<User>::iterator it = _users.begin(); it != _users.end();) {
            if (!it->alive()) {
                _debug << Color::RED << "User " << it->name() << " has timed out." << Log::endl;
                std::set<User>::iterator next = it; ++next;
                removeUser(it);
                it = next;
            } else {
                ++it;
            }
        }

        // Save user data
        if (_time - _lastSave >= SAVE_FREQUENCY) {
            for (const User &user : _users) {
                writeUserData(user);
            }
            saveData();
            _lastSave = _time;
        }

        // Update users
        for (const User &user : _users)
            const_cast<User&>(user).update(timeElapsed, *this);

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

    // Save all user data
    for(const User &user : _users){
        writeUserData(user);
    }
}

void Server::draw() const{
    renderer.clear();
    _debug.draw();
    renderer.present();
}

void Server::addUser(const Socket &socket, const std::string &name){
    User newUser(name, 0, socket);
    bool userExisted = readUserData(newUser);
    if (!userExisted) {
        newUser.location(mapRand());
        _debug << "New";
    } else {
        _debug << "Existing";
    }
    _debug << " user, " << name << " has logged in." << Log::endl;
    _usernames.insert(name);

    // Send welcome message
    sendMessage(socket, SV_WELCOME);

    // Send new user the map
    sendMessage(newUser.socket(), SV_MAP_SIZE, makeArgs(_mapX, _mapY));
    for (size_t y = 0; y != _mapY; ++y){
        std::ostringstream oss;
        oss << "0," << y << "," << _mapX;
        for (size_t x = 0; x != _mapX; ++x)
            oss << "," << _map[x][y];
        sendMessage(newUser.socket(), SV_TERRAIN, oss.str());
    }

    // Send him everybody else's location
    for (const User &user : _users)
        sendMessage(newUser.socket(), SV_LOCATION, user.makeLocationCommand());

    // Send him object locations
    for (const Object &obj : _objects) {
        assert (obj.type());
        sendMessage(newUser.socket(), SV_OBJECT,
                    makeArgs(obj.serial(), obj.location().x, obj.location().y, obj.type()->id()));
    }

    // Send him his inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        if (newUser.inventory(i).first != "none")
            sendMessage(socket, SV_INVENTORY,
                        makeArgs(i, newUser.inventory(i).first, newUser.inventory(i).second));
    }

    // Add new user to list, and broadcast his location
    _users.insert(newUser);
    broadcast(SV_LOCATION, newUser.makeLocationCommand());
}

void Server::removeUser(std::set<User>::iterator &it){
        // Broadcast message
        broadcast(SV_USER_DISCONNECTED, it->name());

        // Save user data
        writeUserData(*it);

        _usernames.erase(it->name());
        _users.erase(it);
}

void Server::removeUser(const Socket &socket){
    std::set<User>::iterator it = _users.find(socket);
    if (it != _users.end())
        removeUser(it);
}

void Server::handleMessage(const Socket &client, const std::string &msg){
    int msgCode;
    char del;
    static char buffer[BUFFER_SIZE+1];
    std::istringstream iss(msg);
    User *user = 0;
    while (iss.peek() == '[') {
        iss >> del >> msgCode >> del;
        
        // Discard message if this client has not yet sent CL_I_AM
        std::set<User>::iterator it = _users.find(client);
        if (it == _users.end() && msgCode != CL_I_AM) {
            continue;
        }
        if (msgCode != CL_I_AM) {
            User &userRef = const_cast<User&>(*it);
            user = &userRef;
            user->contact();
        }

        switch(msgCode) {

        case CL_PING:
        {
            Uint32 timeSent;
            iss >> timeSent  >> del;
            if (del != ']')
                return;
            sendMessage(user->socket(), SV_PING_REPLY, makeArgs(timeSent));
            break;
        }

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
            for (char c : name){
                if (c < 'a' || c > 'z') {
                    sendMessage(client, SV_INVALID_USERNAME);
                    invalid = true;
                    break;
                }
            }
            if (invalid)
                break;

            // Check that user isn't already logged in
            if (_usernames.find(name) != _usernames.end()) {
                sendMessage(client, SV_DUPLICATE_USERNAME);
                invalid = true;
                break;
            }

            addUser(client, name);

            break;
        }

        case CL_LOCATION:
        {
            double x, y;
            iss >> x >> del >> y >> del;
            if (del != ']')
                return;
            user->cancelAction(*this);
            user->updateLocation(Point(x, y), *this);
            broadcast(SV_LOCATION, user->makeLocationCommand());
            break;
        }

        case CL_CRAFT:
        {
            std::string id;
            iss.get(buffer, BUFFER_SIZE, ']');
            id = std::string(buffer);
            iss >> del;
            if (del != ']')
                return;
            user->cancelAction(*this);
            std::set<Item>::const_iterator it = _items.find(id);
            if (it == _items.end()) {
                sendMessage(client, SV_INVALID_ITEM);
                break;
            }
            if (!it->isCraftable()) {
                sendMessage(client, SV_CANNOT_CRAFT);
                break;
            }
            if (!user->hasMaterials(*it)) {
                sendMessage(client, SV_NEED_MATERIALS);
                break;
            }
            user->actionCraft(*it);
            sendMessage(client, SV_ACTION_STARTED, makeArgs(it->craftTime()));
            break;
        }

        case CL_CONSTRUCT:
        {
            size_t slot;
            double x, y;
            iss >> slot >> del >> x >> del >> y >> del;
            if (del != ']')
                return;
            user->cancelAction(*this);
            if (slot >= User::INVENTORY_SIZE) {
                sendMessage(client, SV_INVALID_SLOT);
                break;
            }
            const std::pair<std::string, size_t> &invSlot = user->inventory(slot);
            if (invSlot.first == "none") {
                sendMessage(client, SV_EMPTY_SLOT);
                break;
            }
            std::set<Item>::const_iterator it = _items.find(invSlot.first);
            if (it == _items.end()) {
                break;
            }
            if (!it->isClass("structure")) {
                sendMessage(client, SV_CANNOT_CONSTRUCT);
                break;
            }
            Point location(x, y);
            if (distance(user->location(), location) > ACTION_DISTANCE) {
                sendMessage(client, SV_TOO_FAR);
                break;
            }
            //user->actionConstruct([object], location, slot);
            //sendMessage(client, SV_ACTION_STARTED, makeArgs(it->craftTime()));
            break;
        }

        case CL_CANCEL_ACTION:
        {
            iss >> del;
            if (del != ']')
                return;
            user->cancelAction(*this);
            break;
        }

        case CL_GATHER:
        {
            int serial;
            iss >> serial >> del;
            if (del != ']')
                return;
            user->cancelAction(*this);
            std::set<Object>::const_iterator it = _objects.find(serial);
            if (it == _objects.end()) {
                sendMessage(client, SV_DOESNT_EXIST);
            } else if (distance(user->location(), it->location()) > ACTION_DISTANCE) {
                sendMessage(client, SV_TOO_FAR);
            } else {
                const Object &obj = *it;
                // Check that the user meets the requirements
                assert (obj.type());
                const std::string &gatherReq = obj.type()->gatherReq();
                if (gatherReq != "none" && !user->hasItemClass(gatherReq, *this)) {
                    sendMessage(client, SV_ITEM_NEEDED, gatherReq);
                    break;
                }
                user->actionTarget(&obj);
                sendMessage(client, SV_ACTION_STARTED, makeArgs(obj.type()->actionTime()));
            }
            break;
        }

        default:
            _debug << Color::RED << "Unhandled message: " << msg << Log::endl;
        }
    }
}

void Server::broadcast(MessageCode msgCode, const std::string &args){
    for (const User &user : _users){
        sendMessage(user.socket(), msgCode, args);
    }
}

void Server::gatherObject(size_t serial, User &user){
    // Give wood to user
    std::set<Object>::iterator it = _objects.find(serial);
    Object &obj = const_cast<Object &>(*it);
    size_t slot = user.giveItem(*_items.find(std::string("wood")));
    if (slot == User::INVENTORY_SIZE) {
        sendMessage(user.socket(), SV_INVENTORY_FULL);
        return;
    }
    sendMessage(user.socket(), SV_INVENTORY, makeArgs(slot, "wood", user.inventory(slot).second));
    // Remove tree if empty
    obj.decrementWood();
    if (it->wood() == 0) {
        // Ensure no other users are targeting this object, as it will be removed.
        for (const User &otherUserConst : _users) {
            User &otherUser = const_cast<User &>(otherUserConst);
            if (&otherUser != &user &&
                otherUser.actionTarget() &&
                otherUser.actionTarget()->serial() == serial) {

                user.actionTarget(0);
                sendMessage(otherUser.socket(), SV_DOESNT_EXIST);
            }
        }
        broadcast(SV_REMOVE_OBJECT, makeArgs(serial));
        _objects.erase(it);
    }

    saveData();
}

bool Server::readUserData(User &user){
    std::string filename = std::string("Users/") + user.name() + ".usr";
    std::ifstream fs(filename.c_str());
    if (!fs.good()) // File didn't exist
        return false;
    
    // Location
    user.location(fs);

    // Inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        std::string itemID;
        int quantity;
        fs >> itemID >> quantity;
        user.inventory(i) = std::make_pair(itemID, quantity);
    }

    fs.close();
    return true;
}

void Server::writeUserData(const User &user) const{
    std::string filename = std::string("Users/") + user.name() + ".usr";
    std::ofstream fs(filename.c_str());
    
    // Location
    fs << user.location().x << ' ' << user.location().y << std::endl;

    // Inventory
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        fs << user.inventory(i).first << ' ' << user.inventory(i).second << std::endl;
    }

    fs.close();
}


void Server::sendMessage(const Socket &dstSocket, MessageCode msgCode,
                         const std::string &args) const{
    // Compile message
    std::ostringstream oss;
    oss << '[' << msgCode;
    if (args != "")
        oss << ',' << args;
    oss << ']';

    // Send message
    _socket.sendMessage(oss.str(), dstSocket);
}

void Server::loadData(){
    // Load data
    _items.insert(Item("none", "none"));
    _items.insert(Item("wood", "wood", 5));

    Item i("axe", "wooden axe");
    i.addClass("axe");
    i.addMaterial("wood", 3);
    i.craftTime(10);
    _items.insert(i);

    i = Item("chest", "wooden chest", 10);
    i.addClass("structure");
    i.addClass("container");
    i.addMaterial("wood", 5);
    i.craftTime(10);
    _items.insert(i);

    std::ifstream fs("Data/objectTypes.dat");
    if (!fs.good()) {
        _debug("Object-types file invalid; aborting data load.", Color::RED);
        return;
    }
    int numObjTypes;
    fs >> numObjTypes;
    for (int i = 0; i != numObjTypes; ++i) {
        std::string id;
        Uint32 actionTime;
        size_t wood;
        std::string gatherReq;
        fs >> id >> actionTime >> wood >> gatherReq;
        _objectTypes.insert(ObjectType(id, actionTime, wood, gatherReq));
    }
    if (!fs.good()) {
        _debug("Object-types file invalid; aborting data load.", Color::RED);
        return;
    }
    fs.close();

    // Detect/load state
    do {
        if (cmdLineArgs.contains("new"))
            break;

        fs.open("World/map.dat");
        if (!fs.good())
            break;
        fs >> _mapX >> _mapY;
        _map = std::vector<std::vector<size_t> >(_mapX);
        for (size_t x = 0; x != _mapX; ++x)
            _map[x] = std::vector<size_t>(_mapY, 0);
        for (size_t y = 0; y != _mapY; ++y)
            for (size_t x = 0; x != _mapX; ++x)
                fs >> _map[x][y];
        if (!fs.good())
            break;
        fs.close();

        fs.open("World/objects.dat");
        if (!fs.good())
            break;
        int numObjects;
        fs >> numObjects;
        for (int i = 0; i != numObjects; ++i) {
            std::string type;
            Point p;
            size_t wood;
            fs >> type >> p.x >> p.y >> wood;
            std::set<ObjectType>::const_iterator it = _objectTypes.find(type);
            if (it == _objectTypes.end()) {
                _debug << Color::RED << "Object with invalid type '" << type <<
                       "' cannot be loaded." << Log::endl;
                continue;
            }
            _objects.insert(Object(&*it, p, wood));
        }
        if (!fs.good())
            break;
        fs.close();

        return;
    } while (false);

    _debug("No/invalid world data detected; generating new world.", Color::YELLOW);
    generateWorld();
}

void Server::saveData() const{
    std::ofstream fs("World/map.dat");
    fs << _mapX << '\n' << _mapY << '\n';
    for (size_t y = 0; y != _mapY; ++y){
        for (size_t x = 0; x != _mapX; ++x){
            fs << ' ' << _map[x][y];
        }
        fs << '\n';
    }
    fs.close();

    fs.open("World/objects.dat");
    fs << _objects.size() << '\n';
    for (const Object &obj : _objects) {
        assert (obj.type());
        fs << obj.type()->id() << ' '
           << obj.location().x << ' '
           << obj.location().y << ' '
           << obj.wood() << '\n';
    }
    fs.close();
}

size_t Server::findTile(const Point &p) const{
    size_t y = static_cast<size_t>(p.y / TILE_H);
    double rawX = p.x;
    if (y % 2 == 1)
        rawX -= TILE_W/2;
    size_t x = static_cast<size_t>(rawX / TILE_W);
    if (x >= _mapX || y >= _mapY)
        assert(false);
    return _map[x][y];
}

void Server::generateWorld(){
    _mapX = 30;
    _mapY = 30;

    // Grass by default
    _map = std::vector<std::vector<size_t> >(_mapX);
    for (size_t x = 0; x != _mapX; ++x){
        _map[x] = std::vector<size_t>(_mapY);
        for (size_t y = 0; y != _mapY; ++y)
            _map[x][y] = 0;
    }

    // Stone in circles
    for (int i = 0; i != 5; ++i) {
        size_t centerX = rand() % _mapX;
        size_t centerY = rand() % _mapY;
        for (size_t x = 0; x != _mapX; ++x)
            for (size_t y = 0; y != _mapY; ++y) {
                Point thisTile(x, y);
                if (y % 2 == 1)
                    thisTile.x -= .5;
                double dist = distance(Point(centerX, centerY), thisTile);
                if (dist <= 4)
                    _map[x][y] = 1;
            }
    }

    // Roads
    for (int i = 0; i != 2; ++i) {
        Point
            start(rand() % _mapX, rand() % _mapY),
            end(rand() % _mapX, rand() % _mapY);
        for (size_t x = 0; x != _mapX; ++x)
            for (size_t y = 0; y != _mapY; ++y) {
                Point thisTile(x, y);
                if (y % 2 == 1)
                    thisTile.x -= .5;
                double dist = distance(thisTile, start, end);
                if (dist <= 1)
                    _map[x][y] = 2;
            }
    }

    // River: randomish line
    Point
        start(rand() % _mapX, rand() % _mapY),
        end(rand() % _mapX, rand() % _mapY);
    for (size_t x = 0; x != _mapX; ++x)
        for (size_t y = 0; y != _mapY; ++y) {
            Point thisTile(x, y);
            if (y % 2 == 1)
                thisTile.x -= .5;
            double dist = distance(thisTile, start, end) + randDouble() * 2 - 1;
            if (dist <= 0.5)
                _map[x][y] = 3;
            else if (dist <= 2)
                _map[x][y] = 4;
        }

    const ObjectType *branch = &*_objectTypes.find(std::string("branch"));
    for (int i = 0; i != 30; ++i){
        Point loc;
        size_t tile;
        do {
            loc = mapRand();
            tile = findTile(loc);
        } while (tile == 3 || tile == 4); // Forbidden on water
        _objects.insert(Object(branch, loc));
    }

    const ObjectType *tree = &*_objectTypes.find(std::string("tree"));
    for (int i = 0; i != 10; ++i) {
        Point loc;
        size_t tile;
        do {
            loc = mapRand();
            tile = findTile(loc);
        } while (tile == 3 || tile == 4); // Forbidden on water
        _objects.insert(Object(tree, loc));
    }

    const ObjectType *chest = &*_objectTypes.find(std::string("chest"));
    for (int i = 0; i != 10; ++i) {
        Point loc;
        size_t tile;
        do {
            loc = mapRand();
            tile = findTile(loc);
        } while (tile == 3 || tile == 4); // Forbidden on water
        _objects.insert(Object(chest, loc));
    }
}

Point Server::mapRand() const{
    return Point(randDouble() * (_mapX - 0.5) * TILE_W,
                 randDouble() * _mapY * TILE_H);
}

bool Server::itemIsClass(const std::string &itemID, const std::string &className) const{
    std::set<Item>::const_iterator it = _items.find(itemID);
    if (it == _items.end())
        return false;
    return it->isClass(className);
}
