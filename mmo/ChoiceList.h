// (C) 2015 Tim Gurto

#ifndef CHOICE_LIST_H
#define CHOICE_LIST_H

#include <string>

#include "List.h"
#include "ShadowBox.h"

// A list of elements with IDs; up to one item can be chosen at a time.
class ChoiceList : public List{
    std::string _selectedID;
    std::string _mouseOverID;
    std::string _mouseDownID;
    ShadowBox
        *_selectedBox,
        *_mouseOverBox,
        *_mouseDownBox;

    const std::string &getIdFromMouse(double mouseY) const;

    static void markMouseDown(Element &e, const Point &mousePos);
    static void toggle(Element &e, const Point &mousePos);
    static void markMouseOver(Element &e, const Point &mousePos);

public:
    ChoiceList(const SDL_Rect &rect, int childHeight);

    const std::string &getSelected() { return _selectedID; }
    
    virtual void refresh();
};

#endif