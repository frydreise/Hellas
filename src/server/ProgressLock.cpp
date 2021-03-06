#include <set>

#include "ProgressLock.h"
#include "Server.h"

ProgressLock::locksByType_t ProgressLock::locksByType;
std::set<ProgressLock> ProgressLock::stagedLocks;

ProgressLock::ProgressLock(Type triggerType, const std::string &triggerID,
                           Type effectType, const std::string &effectID,
                           double chance):
_triggerType(triggerType),
_triggerID(triggerID),
_effectType(effectType),
_effectID(effectID),
_chance(chance)
{}

void ProgressLock::registerStagedLocks(){
    for (const ProgressLock &stagedLock : stagedLocks){
        ProgressLock lock = stagedLock;

        const Server &server = Server::instance();
        switch (lock._triggerType){
            case ITEM:
            case GATHER:
            {
                auto it = server._items.find(lock._triggerID);
                if (it != server._items.end())
                    lock._trigger = &*it;
                break;
            }
            case CONSTRUCTION:
                lock._trigger = server.findObjectTypeByName(lock._triggerID);
                break;
            case RECIPE:
            {
                auto it = server._recipes.find(lock._triggerID);
                if (it != server._recipes.end())
                    lock._trigger = &*it;
                break;
            }
            default:
                ;
        }

        if (lock._trigger == nullptr){
            server._debug << Color::RED << "Invalid progress trigger: '" << lock._triggerID << "'" << Log::endl;
            continue;
        }

        switch (lock._effectType){
        case RECIPE:
        {
            auto it = server._recipes.find(lock._effectID);
            if (it != server._recipes.end())
                lock._effect = &*it;
            break;
        }
        case CONSTRUCTION:
            lock._effect = server.findObjectTypeByName(lock._effectID);
            break;
        default:
            ;
        }

        if (lock._effect == nullptr){
            server._debug << Color::RED << "Invalid progress effect: '" << lock._effectID << "'" << Log::endl;
            continue;
        }

        locksByType[lock._triggerType].insert(std::make_pair(lock._trigger, lock));
    }
}

void ProgressLock::triggerUnlocks(User &user, Type triggerType, const void *trigger){
    const locks_t &locks = locksByType[triggerType];
    auto toUnlock = locks.equal_range(trigger);

    std::set<const std::string>
        newRecipes,
        newBuilds;

    for (auto it = toUnlock.first; it != toUnlock.second; ++it){
        const ProgressLock &lock = it->second;

        bool shouldUnlock = randDouble() <= lock._chance;
        if (!shouldUnlock)
            continue;

        std::string id;
        switch (lock._effectType){
        case RECIPE:
            id = reinterpret_cast<const Recipe *>(lock._effect)->id();
            if (user.knowsRecipe(id))
                continue;
            newRecipes.insert(id);
            user.addRecipe(id);
            break;
        case CONSTRUCTION:
             id = reinterpret_cast<const ObjectType *>(lock._effect)->id();
            if (user.knowsConstruction(id))
                continue;
            newBuilds.insert(id);
            user.addConstruction(id);
            break;
        default:
            ;
        }
    }

    const Server &server = Server::instance();
            
    if (!newRecipes.empty()){ // New recipes unlocked!
        std::string args = makeArgs(newRecipes.size());
        for (const std::string &id : newRecipes)
            args = makeArgs(args, id);
        server.sendMessage(user.socket(), SV_NEW_RECIPES, args);
    }

    if (!newBuilds.empty()){ // New constructions unlocked!
        std::string args = makeArgs(newBuilds.size());
        for (const std::string &id : newBuilds)
            args = makeArgs(args, id);
        server.sendMessage(user.socket(), SV_NEW_CONSTRUCTIONS, args);
    }
}

// Order doesnt' really matter, as long as it's a proper ordering.
bool ProgressLock::operator<(const ProgressLock &rhs) const{
    if (_triggerType != rhs._triggerType)
        return _triggerType < rhs._triggerType;
    if (_effectType != rhs._effectType)
        return _effectType < rhs._effectType;
    if (_triggerID != rhs._triggerID)
        return _triggerID < rhs._triggerID;
    return _effectID < rhs._effectID;
}
