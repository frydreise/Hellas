// (C) 2015-2016 Tim Gurto

#include "ServerItem.h"
#include "ObjectType.h"

ServerItem::ServerItem(const std::string &idArg):
Item(idArg),
_stackSize(0),
_constructsObject(nullptr){}

bool vectHasSpace(const ServerItem::vect_t &vect, const ServerItem *item, size_t qty){
    for (size_t i = 0; i != vect.size(); ++i) {
        if (vect[i].first == nullptr) {
            if (qty <= item->stackSize())
                return true;
            qty -= item->stackSize();
        } else if (vect[i].first == item) {
            size_t spaceAvailable = item->stackSize() - vect[i].second;
            if (qty <= spaceAvailable)
                return true;
            qty -= spaceAvailable;
        } else
            continue;
    }
    return false;
}

bool operator<=(const ItemSet &itemSet, const ServerItem::vect_t &vect){
    ItemSet remaining = itemSet;
    for (size_t i = 0; i != vect.size(); ++i){
        const std::pair<const ServerItem *, size_t> &invSlot = vect[i];
        remaining.remove(invSlot.first, invSlot.second);
        if (remaining.isEmpty())
            return true;
    }
    return false;
}

bool operator>(const ItemSet &itemSet, const ServerItem::vect_t &vect) {
    return ! (itemSet <= vect);
}

bool operator>(const ServerItem::vect_t &vect, const ItemSet &itemSet) {
    return ! (itemSet <= vect);
}

const ServerItem *toServerItem(const Item *item){
    return dynamic_cast<const ServerItem *>(item);
}
