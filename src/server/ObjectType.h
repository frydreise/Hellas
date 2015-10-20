// (C) 2015 Tim Gurto

#ifndef OBJECT_TYPE_H
#define OBJECT_TYPE_H

#include <SDL.h>
#include <string>

#include "../Rect.h"
#include "Yield.h"

// Describes a class of Objects, the "instances" of which share common properties
class ObjectType{
    std::string _id;
    Uint32 _actionTime;
    Uint32 _constructionTime;

    // To gather objects of this type, a user must have an item of the following class
    std::string _gatherReq;

    Yield _yield; // If gatherable.

    bool _collides; // false by default; true if any collisionRect is specified.
    Rect _collisionRect; // Relative to position

public:
    ObjectType(const std::string &id);

    void actionTime(Uint32 t) { _actionTime = t; }
    const std::string &gatherReq() const { return _gatherReq; }
    void gatherReq(const std::string &req) { _gatherReq = req; }

    const std::string &id() const { return _id; }
    Uint32 actionTime() const { return _actionTime; }
    Uint32 constructionTime() const { return _constructionTime; }
    void constructionTime(Uint32 t) { _constructionTime = t; }
    const Yield &yield() const { return _yield; }
    bool collides() const { return _collides; }
    const Rect &collisionRect() const { return _collisionRect; }
    void collisionRect(const Rect &r) { _collisionRect = r; _collides = true; }

    bool operator<(const ObjectType &rhs) const { return _id < rhs._id; }

    void addYield(const Item *item, double initMean,
                  double initSD, double gatherMean, double gatherSD);
};

#endif