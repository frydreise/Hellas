// (C) 2015 Tim Gurto

#include "Recipe.h"

Recipe::Recipe(const std::string &id):
_id(id),
_product(0),
_time(0){}

void Recipe::addMaterial(const Item *item, size_t qty){
    _materials.add(item, qty);
}