// (C) 2015 Tim Gurto

#ifndef COLLISION_CHUNK_H
#define COLLISION_CHUNK_H

#include <map>

class Object;

// A subdivision of the map, used
class CollisionChunk{
    std::map<size_t, const Object *> _objects; // Sorted by serial, for fast removal

public:
    void addObject(const Object *obj);
    void removeObject(size_t serial);
    const std::map<size_t, const Object *> objects() const { return _objects; }
};

typedef std::map<size_t, std::map<size_t, CollisionChunk> > CollisionGrid;

#endif
