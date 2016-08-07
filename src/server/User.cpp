// (C) 2015-2016 Tim Gurto

#include <cassert>
#include <sstream>

#include "Item.h"
#include "ItemSet.h"
#include "ObjectType.h"
#include "Server.h"
#include "User.h"
#include "../Socket.h"
#include "../messageCodes.h"
#include "../util.h"

const size_t User::INVENTORY_SIZE = 10;

const ObjectType User::OBJECT_TYPE(Rect(-5, -2, 10, 4));

const unsigned User::ATTACK = 5;

User::User(const std::string &name, const Point &loc, const Socket &socket):
_name(name),
_socket(socket),
_location(loc),

_action(NO_ACTION),
_actionTime(0),
_actionObject(nullptr),
_actionRecipe(nullptr),
_actionObjectType(nullptr),
_actionSlot(INVENTORY_SIZE),
_actionLocation(0, 0),

_inventory(INVENTORY_SIZE),
_lastLocUpdate(SDL_GetTicks()),
_lastContact(SDL_GetTicks()){
    for (size_t i = 0; i != INVENTORY_SIZE; ++i)
        _inventory[i] = std::make_pair<const Item *, size_t>(0, 0);
}

User::User(const Socket &rhs):
_socket(rhs){}

User::User(const Point &loc):
_location(loc){}

bool User::compareXThenSerial::operator()( const User *a, const User *b){
    if (a->_location.x != b->_location.x)
        return a->_location.x < b->_location.x;
    return a->_socket < b->_socket; // Just need something unique.
}

bool User::compareYThenSerial::operator()( const User *a, const User *b){
    if (a->_location.y != b->_location.y)
        return a->_location.y < b->_location.y;
    return a->_socket < b->_socket; // Just need something unique.
}

const std::pair<const Item *, size_t> &User::inventory(size_t index) const{
    return _inventory[index];
}

std::pair<const Item *, size_t> &User::inventory(size_t index){
    return _inventory[index];
}

std::string User::makeLocationCommand() const{
    return makeArgs(_name, _location.x, _location.y);
}

void User::updateLocation(const Point &dest){
    const ms_t newTime = SDL_GetTicks();
    ms_t timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    // Max legal distance: straight line
    const double maxLegalDistance = min(Server::MAX_TIME_BETWEEN_LOCATION_UPDATES,
                                        timeElapsed + 100)
                                    / 1000.0 * Server::MOVEMENT_SPEED;
    Point interpolated = interpolate(_location, dest, maxLegalDistance);

    Point newDest = interpolated;
    Server &server = *Server::_instance;
    if (!server.isLocationValid(newDest, OBJECT_TYPE, 0, this)) {
        newDest = _location;
        static const double ACCURACY = 0.5;
        Point testPoint = _location;
        const bool xDeltaPositive = _location.x < interpolated.x;
        do {
            newDest.x = testPoint.x;
            testPoint.x = xDeltaPositive ? (testPoint.x) + ACCURACY : (testPoint.x - ACCURACY);
        } while ((xDeltaPositive ? (testPoint.x <= interpolated.x) :
                                   (testPoint.x >= interpolated.x)) &&
                 server.isLocationValid(testPoint, OBJECT_TYPE, nullptr, this));
        const bool yDeltaPositive = _location.y < interpolated.y;
        testPoint.x = newDest.x; // Keep it valid for y testing.
        do {
            newDest.y = testPoint.y;
            testPoint.y = yDeltaPositive ? (testPoint.y + ACCURACY) : (testPoint.y - ACCURACY);
        } while ((yDeltaPositive ? (testPoint.y <= interpolated.y) :
                                   (testPoint.y >= interpolated.y)) &&
                 server.isLocationValid(testPoint, OBJECT_TYPE, nullptr, this));
    }

    // Tell user about any additional objects he can now see
    std::list<const Object *> objectsToSend;
    double left, right, top, bottom;
    if (newDest.x > _location.x){ // Moved right
        left = _location.x + Server::CULL_DISTANCE;
        right = newDest.x + Server::CULL_DISTANCE;
    } else { // Moved left
        left = newDest.x - Server::CULL_DISTANCE;
        right = _location.x - Server::CULL_DISTANCE;
    }
    if (newDest.y > _location.y){ // Moved down
        top = _location.y + Server::CULL_DISTANCE;
        bottom = newDest.y + Server::CULL_DISTANCE;
    } else { // Moved up
        top = newDest.y - Server::CULL_DISTANCE;
        bottom = _location.y - Server::CULL_DISTANCE;
    }
    auto loX = server._objectsByX.lower_bound(&Object(Point(left, 0)));
    auto hiX = server._objectsByX.upper_bound(&Object(Point(right, 0)));
    auto loY = server._objectsByY.lower_bound(&Object(Point(0, top)));
    auto hiY = server._objectsByY.upper_bound(&Object(Point(0, bottom)));
    for (auto it = loX; it != hiX; ++it){
        if (abs((*it)->location().y - newDest.y) <= Server::CULL_DISTANCE)
            objectsToSend.push_back(*it);
    }
    for (auto it = loY; it != hiY; ++it){
        double objX = (*it)->location().x;
        if (newDest.x > _location.x){ // Don't count objects twice
            if (objX > left)
                continue;
        } else {
            if (objX < right)
                continue;
        }
        if (abs(objX - newDest.x) <= Server::CULL_DISTANCE)
            objectsToSend.push_back(*it);
    }
    for (const Object *objP : objectsToSend){
        server.sendMessage(socket(), SV_OBJECT,
                           makeArgs(objP->serial(), objP->location().x, objP->location().y,
                                    objP->type()->id()));
        if (!objP->owner().empty())
            server.sendMessage(socket(), SV_OWNER, makeArgs(objP->serial(), objP->owner()));
    }

    Point oldLoc = _location;

    // Remove from location-indexed trees
    if (newDest.x != oldLoc.x)
        server._usersByX.erase(this);
    if (newDest.y != oldLoc.y)
        server._usersByY.erase(this);

    _location = newDest;

    // Re-insert into location-indexed trees
    if (newDest.x != oldLoc.x)
        server._usersByX.insert(this);
    if (newDest.y != oldLoc.y)
        server._usersByY.insert(this);
}

void User::contact(){
    _lastContact = SDL_GetTicks();
}

bool User::alive() const{
    return SDL_GetTicks() - _lastContact <= Server::CLIENT_TIMEOUT;
}

size_t User::giveItem(const Item *item, size_t quantity){
    // First pass: partial stacks
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first != item)
            continue;
        size_t spaceAvailable = item->stackSize() - _inventory[i].second;
        if (spaceAvailable > 0) {
            size_t qtyInThisSlot = min(spaceAvailable, quantity);
            _inventory[i].second += qtyInThisSlot;
            Server::instance().sendInventoryMessage(*this, i);
            quantity -= qtyInThisSlot;
        }
        if (quantity == 0)
            return 0;
    }

    // Second pass: empty slots
    for (size_t i = 0; i != INVENTORY_SIZE; ++i) {
        if (_inventory[i].first != nullptr)
            continue;
        size_t qtyInThisSlot = min(item->stackSize(), quantity);
        _inventory[i].first = item;
        _inventory[i].second = qtyInThisSlot;
        Server::instance().sendInventoryMessage(*this, i);
        quantity -= qtyInThisSlot;
        if (quantity == 0)
            return 0;
    }
    return quantity;
}

void User::cancelAction() {
    if (_action != NO_ACTION) {
        Server::instance().sendMessage(_socket, SV_ACTION_INTERRUPTED);
        _action = NO_ACTION;
    }
}

void User::beginGathering(Object *obj){
    _action = GATHER;
    _actionObject = obj;
    assert(obj->type());
    _actionTime = obj->type()->gatherTime();
}

void User::beginCrafting(const Recipe &recipe){
    _action = CRAFT;
    _actionRecipe = &recipe;
    _actionTime = recipe.time();
}

void User::beginConstructing(const ObjectType &obj, const Point &location, size_t slot){
    _action = CONSTRUCT;
    _actionObjectType = &obj;
    _actionTime = obj.constructionTime();
    _actionSlot = slot;
    _actionLocation = location;
}

void User::beginDeconstructing(Object &obj){
    _action = DECONSTRUCT;
    _actionObject = &obj;
    _actionTime = obj.type()->deconstructionTime();
}

bool User::hasItems(const ItemSet &items) const{
    ItemSet remaining = items;
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        const std::pair<const Item *, size_t> &invSlot = _inventory[i];
        remaining.remove(invSlot.first, invSlot.second);
        if (remaining.isEmpty())
            return true;
    }
    return false;
}

bool User::hasTool(const std::string &className) const{
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i) {
        const Item *item = _inventory[i].first;
        if (item && item->isClass(className))
            return true;
    }

    // Check nearby objects
    auto superChunk = Server::_instance->getCollisionSuperChunk(_location);
    for (CollisionChunk *chunk : superChunk)
        for (const auto &ret : chunk->objects()) {
            const Object *pObj = ret.second;
            if (pObj->type()->isClass(className) &&
                distance(pObj->collisionRect(), collisionRect()) < Server::ACTION_DISTANCE)
                return true;
        }

    return false;
}

bool User::hasTools(const std::set<std::string> &classes) const{
    for (const std::string &className : classes)
        if (!hasTool(className))
            return false;
    return true;
}

void User::removeItems(const ItemSet &items) {
    std::set<size_t> invSlotsChanged;
    ItemSet remaining = items;
    for (size_t i = 0; i != User::INVENTORY_SIZE; ++i){
        std::pair<const Item *, size_t> &invSlot = _inventory[i];
        if (remaining.contains(invSlot.first)) {
            size_t itemsToRemove = min(invSlot.second, remaining[invSlot.first]);
            remaining.remove(invSlot.first, itemsToRemove);
            _inventory[i].second -= itemsToRemove;
            if (_inventory[i].second == 0)
                _inventory[i].first = nullptr;
            invSlotsChanged.insert(i);
            if (remaining.isEmpty())
                break;
        }
    }
    for (size_t slotNum : invSlotsChanged) {
        const std::pair<const Item *, size_t> &slot = _inventory[slotNum];
        std::string id = slot.first ? slot.first->id() : "none";
        Server::instance().sendInventoryMessage(*this, slotNum);
    }
}

void User::update(ms_t timeElapsed){
    if (_action == NO_ACTION)
        return;
    if (_actionTime > timeElapsed) {
        _actionTime -= timeElapsed;
        return;
    }

    Server &server = *Server::_instance;
    switch(_action){
    case GATHER:
        server.gatherObject(_actionObject->serial(), *this);
        break;

    case CRAFT:
        if (!vectHasSpace(_inventory, _actionRecipe->product())) {
            server.sendMessage(_socket, SV_INVENTORY_FULL);
            _action = NO_ACTION;
            return;
        }
        // Give user his newly crafted item
        giveItem(_actionRecipe->product(), 1);
        // Remove materials from user's inventory
        removeItems(_actionRecipe->materials());
        break;

    case CONSTRUCT:
    {
        // Create object
        server.addObject(_actionObjectType, _actionLocation, this);
        // Remove item from user's inventory
        std::pair<const Item *, size_t> &slot = _inventory[_actionSlot];
        assert(slot.first->constructsObject() == _actionObjectType);
        --slot.second;
        if (slot.second == 0)
            slot.first = nullptr;
        server.sendInventoryMessage(*this, _actionSlot);
        break;
    }

    case DECONSTRUCT:
    {
        //Check for inventory space
        const Item *item = _actionObject->type()->deconstructsItem();
        if (!vectHasSpace(_inventory, item)){
            server.sendMessage(_socket, SV_INVENTORY_FULL);
            _action = NO_ACTION;
            return;
        }
        // Give user his item
        giveItem(item);
        // Remove object
        server.removeObject(*_actionObject);
        break;
    }

    default:
        assert(false);
    }
    
    server.sendMessage(_socket, SV_ACTION_FINISHED);
    _action = NO_ACTION;
}

const Rect User::collisionRect() const{
    return OBJECT_TYPE.collisionRect() + _location;
}
