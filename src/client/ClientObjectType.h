// (C) 2015 Tim Gurto

#ifndef CLIENT_OBJECT_TYPE_H
#define CLIENT_OBJECT_TYPE_H

#include <string>

#include "EntityType.h"
#include "Texture.h"
#include "../Point.h"
#include "../util.h"

struct Mix_Chunk;

// Describes a class of Entities, the "instances" of which share common properties
class ClientObjectType : public EntityType{
    std::string _id;
    std::string _name;
    bool _canGather; // Whether this represents objects that can be gathered
    Mix_Chunk *_gatherSound;
    Rect _collisionRect;

public:
    ClientObjectType(const std::string &id);
    ~ClientObjectType();

    const std::string &name() const { return _name; }
    void name(const std::string &s) { _name = s; }
    bool canGather() const { return _canGather; }
    void canGather(bool b) { _canGather = b; }
    void gatherSound(const std::string &filename);
    Mix_Chunk *gatherSound() const { return _gatherSound; }
    const Rect &collisionRect() const { return _collisionRect; }
    void collisionRect(const Rect &r) { _collisionRect = r; }

    bool operator<(const ClientObjectType &rhs) const { return _id < rhs._id; }
};

#endif
