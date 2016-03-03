// (C) 2015 Tim Gurto

#include <cassert>

#include "Object.h"
#include "Server.h"
#include "../util.h"

Object::Object(const ObjectType *type, const Point &loc):
_serial(generateSerial()),
_location(loc),
_type(type){
    assert(type);
    if (type->yield()) {
        type->yield().instantiate(_contents);

        // Print contents
        //Server::debug() << "New " << type->id() << " contains "
        //                << _contents.totalQuantity() << " items:";
        //for (auto &pair : _contents)
        //    Server::debug() << " " << pair.second << "*" << pair.first->id();
        //Server::debug() << Log::endl;
    }

    if (type->containerSlots() != 0)
        _container = Item::vect_t(type->containerSlots());

    if (type->merchantSlots() != 0)
        _merchantSlots = std::vector<MerchantSlot>(type->merchantSlots());
}

Object::Object(size_t serial): // For set/map lookup
_serial (serial),
_type(0){}

size_t Object::generateSerial() {
    static size_t currentSerial = 1; // Serial 0 is unavailable, and has special meaning.
    return currentSerial++;
}

void Object::contents(const ItemSet &contents){
    _contents = contents;
}

void Object::removeItem(const Item *item, size_t qty){
    assert (_contents[item] >= qty);
    assert (_contents.totalQuantity() >= qty);
    _contents.remove(item, qty);
}

const Item *Object::chooseGatherItem() const{
    assert(!_contents.isEmpty());
    assert(_contents.totalQuantity() > 0);
    
    // Choose random item, weighted by remaining quantity of each item type.
    size_t i = rand() % _contents.totalQuantity();
    for (auto item : _contents) {
        if (i <= item.second)
            return item.first;
        else
            i -= item.second;
    }
    assert(false);
    return 0;
}

size_t Object::chooseGatherQuantity(const Item *item) const{
    size_t randomQty = _type->yield().generateGatherQuantity(item);
    size_t qty = min<size_t>(randomQty, _contents[item]);
    return qty;
}

bool Object::userHasAccess(const std::string &username) const{
    return
        _owner.empty() ||
        _owner == username;
}

void Object::removeItems(const ItemSet &items) {
    std::set<size_t> invSlotsChanged;
    ItemSet remaining = items;
    for (size_t i = 0; i != _container.size(); ++i){
        std::pair<const Item *, size_t> &invSlot = _container[i];
        if (remaining.contains(invSlot.first)) {
            size_t itemsToRemove = min(invSlot.second, remaining[invSlot.first]);
            remaining.remove(invSlot.first, itemsToRemove);
            _container[i].second -= itemsToRemove;
            if (_container[i].second == 0)
                _container[i].first = 0;
            invSlotsChanged.insert(i);
            if (remaining.isEmpty())
                break;
        }
    }
    for (const std::string &username : _watchers) {
        const User &user = Server::instance().getUserByName(username);
        for (size_t slotNum : invSlotsChanged) {
            Server::instance().sendInventoryMessage(user, _serial, slotNum);
        }
    }
}



void Object::giveItem(const Item *item, size_t qty){
    std::set<size_t> changedSlots;
    // First pass: partial stacks
    for (size_t i = 0; i != _container.size(); ++i) {
        if (_container[i].first != item)
            continue;
        size_t spaceAvailable = item->stackSize() - _container[i].second;
        if (spaceAvailable > 0) {
            size_t qtyInThisSlot = min(spaceAvailable, qty);
            _container[i].second += qtyInThisSlot;
            changedSlots.insert(i);
            qty -= qtyInThisSlot;
        }
        if (qty == 0)
            break;;
    }

    // Second pass: empty slots
    if (qty != 0)
        for (size_t i = 0; i != _container.size(); ++i) {
            if (_container[i].first)
                continue;
            size_t qtyInThisSlot = min(item->stackSize(), qty);
            _container[i].first = item;
            _container[i].second = qtyInThisSlot;
            changedSlots.insert(i);
            qty -= qtyInThisSlot;
            if (qty == 0)
                break;;
        }
    assert(qty == 0);

    for (const std::string &username : _watchers){
        const User &user = Server::instance().getUserByName(username);
        for (size_t slot : changedSlots)
            Server::instance().sendInventoryMessage(user, _serial, slot);
    }
}

void Object::addWatcher(const std::string &username){
    _watchers.insert(username);
    Server::debug() << username << " is now watching an object." << Log::endl;
}

void Object::removeWatcher(const std::string &username){
    _watchers.erase(username);
    Server::debug() << username << " is no longer watching an object." << Log::endl;
}
