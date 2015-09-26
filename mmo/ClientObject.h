// (C) 2015 Tim Gurto

#ifndef CLIENT_OBJECT_H
#define CLIENT_OBJECT_H

#include <sstream>
#include <string>

#include "Entity.h"
#include "Point.h"

// A client-side description of an object
class ClientObject : public Entity{
    size_t _serial;

public:
    ClientObject(const ClientObject &rhs);
    // Serial only: create dummy object, for set searches
    ClientObject(size_t serial, const EntityType &type = EntityType(), const Point &loc = Point());

    bool operator<(const ClientObject &rhs) const { return _serial < rhs._serial; }
    bool operator==(const ClientObject &rhs) const { return _serial == rhs._serial; }

    size_t serial() const { return _serial; }

    virtual void onLeftClick(Client &client) const;
    virtual std::vector<std::string> getTooltipMessages(const Client &client) const;
};

#endif