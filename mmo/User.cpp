#include <sstream>

#include "Client.h"
#include "Server.h"
#include "Socket.h"
#include "User.h"
#include "messageCodes.h"
#include "util.h"

const size_t User::INVENTORY_SIZE = 5;

User::User(const std::string &name, const Point &loc, const Socket &socket):
_name(name),
_location(loc),
_socket(socket),
_inventory(INVENTORY_SIZE, std::make_pair("none", 0)),
_lastLocUpdate(SDL_GetTicks()),
_lastContact(SDL_GetTicks()){}

User::User(const Socket &rhs):
_socket(rhs){}

bool User::operator<(const User &rhs) const{
    return _socket < rhs._socket;
}

const std::string &User::name() const{
    return _name;
}

const Socket &User::socket() const{
    return _socket;
}

const Point &User::location() const{
    return _location;
}

void User::location(const Point &loc){
    _location = loc;
}

void User::location(std::istream &is){
    is >> _location.x >> _location.y;
}

const std::pair<std::string, size_t> &User::inventory(size_t index) const{
    return _inventory[index];
}

std::pair<std::string, size_t> &User::inventory(size_t index){
    return _inventory[index];
}

std::string User::makeLocationCommand() const{
    return makeArgs(_name, _location.x, _location.y);
}

void User::updateLocation(const Point &dest){
    Uint32 newTime = SDL_GetTicks();
    Uint32 timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    // Max legal distance: straight line
    double maxLegalDistance = timeElapsed / 1000.0 * Client::MOVEMENT_SPEED;
    _location = interpolate(_location, dest, maxLegalDistance);
}

void User::contact(){
    _lastContact = SDL_GetTicks();
}

bool User::alive() const{
    return SDL_GetTicks() - _lastContact <= Server::CLIENT_TIMEOUT;
}

size_t User::giveItem(const Item &item){
    size_t emptySlot = INVENTORY_SIZE;
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first == item.id() && _inventory[i].second < item.stackSize()){
            ++_inventory[i].second;
            return i;
        }
        if (_inventory[i].first == "none" && emptySlot == INVENTORY_SIZE)
            emptySlot = i;
    }
    if (emptySlot != INVENTORY_SIZE) {
        _inventory[emptySlot] = std::make_pair(item.id(), 1);
    }
    return emptySlot;
}
