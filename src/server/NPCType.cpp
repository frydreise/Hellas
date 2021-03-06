#include "NPC.h"
#include "NPCType.h"

NPCType::NPCType(const std::string &id, health_t maxHealth):
ObjectType(id),
_maxHealth(maxHealth),
_attack(0),
_attackTime(0)
{
    containerSlots(NPC::LOOT_CAPACITY);
}

void NPCType::addLoot(const ServerItem *item, double mean, double sd){
    _lootTable.addItem(item, mean, sd);
}
