// (C) 2015 Tim Gurto

#include "Line.h"
#include "../Renderer.h"

extern Renderer renderer;

Line::Line(int x, int y, int length, Orientation orientation):
Element(Rect(x, y, 2, 2)),
_orientation(orientation){
    if (_orientation == HORIZONTAL)
        width(length);
    else
        height(length);
}

void Line::refresh(){
    const Rect darkRect = _orientation == HORIZONTAL ? Rect(0, 0, rect().w, 1) :
                                                       Rect(0, 0, 1, rect().h); 
    renderer.setDrawColor(SHADOW_DARK);
    renderer.fillRect(darkRect);

    const Rect lightRect = _orientation == HORIZONTAL ? Rect(0, 1, rect().w, 1) :
                                                        Rect(1, 0, 1, rect().h);
                               
    renderer.setDrawColor(SHADOW_LIGHT);
    renderer.fillRect(lightRect);
}