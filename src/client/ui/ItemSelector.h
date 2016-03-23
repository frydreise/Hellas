// (C) 2016 Tim Gurto

#ifndef ITEM_SELECTOR_H
#define ITEM_SELECTOR_H

#include "Button.h"

class Item;

// A button used to choose an Item, and display that choice.
class ItemSelector : public Button{
    const Item *_item;

    ItemSelector(int x = 0, int y = 0);

    const Item *item() const{ return _item; }
    void item(const Item *item) { _item = item; }
};

#endif