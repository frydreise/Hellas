// (C) 2016 Tim Gurto

#ifndef COMBATANT_H
#define COMBATANT_H

#include "../types.h"

// Abstract class describing something that can participate in combat with another Combatant.
class Combatant{
    health_t _health;

public:
    Combatant(health_t health = 0);
    virtual ~Combatant(){}

    virtual health_t maxHealth() const = 0;

    health_t health() const { return _health; }
    void health(health_t health) { _health = health; }
};

#endif