#include <cassert>

#include "Object.h"
#include "Server.h"

void Object::updateLocation(const Point &dest){
    Server &server = *Server::_instance;
    assert(server.isLocationValid(collisionRect(), this));
    const ms_t newTime = SDL_GetTicks();
    ms_t timeElapsed = newTime - _lastLocUpdate;
    _lastLocUpdate = newTime;

    assert(server.isLocationValid(_location, *type(), this));

    const User *userPtr = nullptr;
    if (classTag() == 'u')
        userPtr = dynamic_cast<const User *>(this);

    // Max legal distance: straight line
    double requestedDistance = distance(_location, dest);
    double distanceToMove;
    Point newDest;
    if (classTag() == 'u' && userPtr->isDriving()){
        distanceToMove = requestedDistance;
        newDest = dest;
    } else {
        const double maxLegalDistance = min(Server::MAX_TIME_BETWEEN_LOCATION_UPDATES,
                                            timeElapsed + 100) / 1000.0 * speed();
        distanceToMove = min(maxLegalDistance, requestedDistance);
        newDest = interpolate(_location, dest, distanceToMove);

        Point rawDisplacement(newDest.x - _location.x,
                              newDest.y - _location.y);
        px_t
            displacementX = toInt(ceil(abs(rawDisplacement.x))),
            displacementY = toInt(ceil(abs(rawDisplacement.y)));
        Rect journeyRect = collisionRect();
        if (rawDisplacement.x < 0)
            journeyRect.x -= displacementX;
        journeyRect.w += displacementX;
        if (rawDisplacement.y < 0)
            journeyRect.y -= displacementY;
        journeyRect.h += displacementY;
        if (!server.isLocationValid(journeyRect, this)) {
            newDest = _location;
            assert(server.isLocationValid(newDest, *type(), this));
            static const double ACCURACY = 0.5;
            Point displacementNorm(rawDisplacement.x / distanceToMove * ACCURACY,
                                   rawDisplacement.y / distanceToMove * ACCURACY);
            for (double segment = ACCURACY; segment <= distanceToMove; segment += ACCURACY){
                Point testDest = newDest;
                testDest.x += displacementNorm.x;
                if (!server.isLocationValid(testDest, *type(), this))
                    break;
                newDest = testDest;
            }
            for (double segment = ACCURACY; segment <= distanceToMove; segment += ACCURACY){
                Point testDest = newDest;
                testDest.y += displacementNorm.y;
                if (!server.isLocationValid(testDest, *type(), this))
                    break;
                newDest = testDest;
            }
        }
    }

    // At this point, the new location has been finalized.  Now new information must be propagated.
    assert(server.isLocationValid(newDest, *type(), this));

    double left, right, top, bottom; // Area newly visible
    double forgetLeft, forgetRight, forgetTop, forgetBottom; // Area newly invisible
    if (newDest.x > _location.x){ // Moved right
        left = _location.x + Server::CULL_DISTANCE;
        right = newDest.x + Server::CULL_DISTANCE;
        forgetLeft = _location.x - Server::CULL_DISTANCE;
        forgetRight = newDest.x - Server::CULL_DISTANCE;
    } else { // Moved left
        left = newDest.x - Server::CULL_DISTANCE;
        right = _location.x - Server::CULL_DISTANCE;
        forgetLeft = newDest.x + Server::CULL_DISTANCE;
        forgetRight = _location.x + Server::CULL_DISTANCE;
    }
    if (newDest.y > _location.y){ // Moved down
        top = _location.y + Server::CULL_DISTANCE;
        bottom = newDest.y + Server::CULL_DISTANCE;
        forgetTop = _location.y - Server::CULL_DISTANCE;
        forgetBottom = newDest.y - Server::CULL_DISTANCE;
    } else { // Moved up
        top = newDest.y - Server::CULL_DISTANCE;
        bottom = _location.y - Server::CULL_DISTANCE;
        forgetTop = newDest.y + Server::CULL_DISTANCE;
        forgetBottom = _location.y + Server::CULL_DISTANCE;
    }
    
    if (classTag() == 'u'){
        auto loX = server._objectsByX.lower_bound(&Object(Point(left, 0)));
        auto hiX = server._objectsByX.upper_bound(&Object(Point(right, 0)));
        auto loY = server._objectsByY.lower_bound(&Object(Point(0, top)));
        auto hiY = server._objectsByY.upper_bound(&Object(Point(0, bottom)));

        server.sendMessage(userPtr->socket(), SV_LOCATION, makeArgs(userPtr->name(),
                                                                    newDest.x, newDest.y));
        // Tell user about any additional objects he can now see
        std::list<const Object *> nearbyObjects;
        for (auto it = loX; it != hiX; ++it){
            if (abs((*it)->location().y - newDest.y) <= Server::CULL_DISTANCE)
                nearbyObjects.push_back(*it);
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
                nearbyObjects.push_back(*it);
        }
        for (const Object *objP : nearbyObjects){
            server.sendObjectInfo(*userPtr, *objP);
        }

    }
    
    // Assemble list of newly nearby users (used at the end of this function)
    std::list<const User *> newlyNearbyUsers;
    {
        auto loX = server._usersByX.lower_bound(&User(Point(left, 0)));
        auto hiX = server._usersByX.upper_bound(&User(Point(right, 0)));
        auto loY = server._usersByY.lower_bound(&User(Point(0, top)));
        auto hiY = server._usersByY.upper_bound(&User(Point(0, bottom)));
        for (auto it = loX; it != hiX; ++it){
            if (abs((*it)->location().y - newDest.y) <= Server::CULL_DISTANCE)
                newlyNearbyUsers.push_back(*it);
        }
        for (auto it = loY; it != hiY; ++it){
            double objX = (*it)->location().x;
            if (newDest.x > _location.x){ // Don't count users twice
                if (objX > left)
                    continue;
            } else {
                if (objX < right)
                    continue;
            }
            if (abs(objX - newDest.x) <= Server::CULL_DISTANCE)
                newlyNearbyUsers.push_back(*it);
        }
    }

    // Tell nearby users that it has moved
    std::string args;
    if (classTag() == 'u')
        args = makeArgs(dynamic_cast<const User *>(this)->name(), newDest.x, newDest.y);
    else
        args = makeArgs(serial(), newDest.x, newDest.y);
    for (const User *userP : server.findUsersInArea(location()))
        if (userP != this){
            if (classTag() == 'u')
                server.sendMessage(userP->socket(), SV_LOCATION, args);
            else
                server.sendMessage(userP->socket(), SV_OBJECT_LOCATION, args);
        }

    // Tell any users it has moved away from to forget about it.
   std::list<const User *> usersToForget;
   {
       auto loX = server._usersByX.lower_bound(&User(Point(forgetLeft, 0)));
       auto hiX = server._usersByX.upper_bound(&User(Point(forgetRight, 0)));
       auto loY = server._usersByY.lower_bound(&User(Point(0, forgetTop)));
       auto hiY = server._usersByY.upper_bound(&User(Point(0, forgetBottom)));
        for (auto it = loX; it != hiX; ++it){
            double userY = (*it)->location().y;
            if (userY - _location.y <= Server::CULL_DISTANCE)
                usersToForget.push_back(*it);
        }
        for (auto it = loY; it != hiY; ++it){
            double userX = (*it)->location().x;
            if (newDest.x > _location.x){ // Don't count users twice.
                if (userX < forgetLeft)
                    continue;
            } else {
                if (userX > forgetRight)
                    continue;
            }
            if (abs(userX - _location.x) <= Server::CULL_DISTANCE)
                usersToForget.push_back(*it);
        }
   }
    for (const User *userP : usersToForget){
        if (classTag() == 'u')
            server.sendMessage(userP->socket(), SV_USER_OUT_OF_RANGE, userP->name());
        else
            server.sendMessage(userP->socket(), SV_OBJECT_OUT_OF_RANGE, makeArgs(serial()));
    }

    Point oldLoc = _location;

    // Remove from location-indexed trees
    if (classTag() == 'u'){
        if (newDest.x != oldLoc.x)
            server._usersByX.erase(userPtr);
        if (newDest.y != oldLoc.y)
            server._usersByY.erase(userPtr);
    }
    if (newDest.x != oldLoc.x)
        server._objectsByX.erase(this);
    if (newDest.y != oldLoc.y)
        server._objectsByY.erase(this);

    _location = newDest;
    
    // Re-insert into location-indexed trees
    if (classTag() == 'u'){
        if (newDest.x != oldLoc.x)
            server._usersByX.insert(userPtr);
        if (newDest.y != oldLoc.y)
            server._usersByY.insert(userPtr);
    }
    if (newDest.x != oldLoc.x)
        server._objectsByX.insert(this);
    if (newDest.y != oldLoc.y)
        server._objectsByY.insert(this);


    // Tell newly nearby users that it exists
    for (const User *userP : newlyNearbyUsers)
        server.sendObjectInfo(*userP, *this);

}
