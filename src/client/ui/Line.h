// (C) 2015 Tim Gurto

#ifndef LINE_H
#define LINE_H

#include "Element.h"

// A straight horizontal or vertical line.
class Line : public Element{
    Orientation _orientation;

    virtual void refresh();

public:
    Line(int x, int y, int length, Orientation _orientation = HORIZONTAL); // x,y = top-left corner.
};

#endif