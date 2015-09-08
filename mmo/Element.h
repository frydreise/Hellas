// (C) 2015 Tim Gurto

#include <SDL.h>
#include <list>

#include "Color.h"
#include "Renderer.h"
#include "Texture.h"

#ifndef ELEMENT_H
#define ELEMENT_H


/*
A UI element, making up part of a Window.
The base class may be used as an invisible container for other Elements, to improve the efficiency
of collision detection.
*/
class Element{
public:
    enum Justification{
        LEFT_JUSTIFIED,
        RIGHT_JUSTIFIED,
        TOP_JUSTIFIED,
        BOTTOM_JUSTIFIED,
        CENTER_JUSTIFIED
    };
    enum Orientation{
        HORIZONTAL,
        VERTICAL
    };

private:
    static TTF_Font *_font;

    static Texture transparentBackground;

    mutable bool _changed; // If true, this element should be refreshed before next being drawn.
    bool _dimensionsChanged; // If true, destroy and recreate texture before next draw.

    bool _visible;

    bool _solidBackground; // If true, fill the entire _rect with BACKGROUND_COLOR.

    SDL_Rect _rect; // Location and dimensions within window

    Element *_parent; // 0 if no parent.

    std::string _id; // Optional ID for finding children.

    // Redraw the Element to its texture, usually after something has changed.
    virtual void refresh();

protected:
    static const Color
        BACKGROUND_COLOR,
        SHADOW_LIGHT,
        SHADOW_DARK,
        FONT_COLOR;

    std::list<Element*> _children;

    Texture _texture; // A memoized image of the element, redrawn only when necessary.
    Point location() const { return Point(_rect.x, _rect.y); }
    virtual void checkIfChanged(); // Allows elements to update their changed status.

    typedef void (*mouseDownFunction_t)(Element &e, const Point &mousePos);
    typedef void (*mouseUpFunction_t)(Element &e, const Point &mousePos);
    typedef void (*mouseMoveFunction_t)(Element &e, const Point &mousePos);
    typedef void (*scrollUpFunction_t)(Element &e);
    typedef void (*scrollDownFunction_t)(Element &e);

    mouseDownFunction_t _mouseDown;
    Element *_mouseDownElement;
    mouseUpFunction_t _mouseUp;
    Element *_mouseUpElement;
    mouseMoveFunction_t _mouseMove;
    Element *_mouseMoveElement;

    scrollUpFunction_t _scrollUp;
    Element *_scrollUpElement;
    scrollUpFunction_t _scrollDown;
    Element *_scrollDownElement;

    void drawChildren() const;

public:
    Element(const SDL_Rect &rect);
    virtual ~Element();
    
    static const Point *absMouse; // Absolute mouse co-ordinates

    bool visible() const { return _visible; }
    static TTF_Font *font() { return _font; }
    static void font(TTF_Font *newFont) { _font = newFont; }
    const SDL_Rect &rect() const { return _rect; }
    void rect(const SDL_Rect &rhs);
    void rect(int x, int y) { _rect.x = x; _rect.y = y; }
    void rect(int x, int y, int w, int h);
    int width() const { return _rect.w; }
    void width(int w);
    int height() const { return _rect.h; }
    void height(int h);
    bool changed() const { return _changed; }
    const std::string &id() const { return _id; }
    void id(const std::string &id) { _id = id; }
    const std::list<Element *> &children() const { return _children; }

    void show() { _visible = true; }
    void hide() { _visible = false; }
    void toggleVisibility() { _visible = !_visible; }
    void fillBackground() { _solidBackground = true; }

    void markChanged() const;

    virtual void addChild(Element *child);
    virtual void clearChildren(); // Delete all children
    virtual Element *findChild(const std::string id); // Find a child by ID, or 0 if not found.

    void makeBackgroundTransparent();

    // e: allows the function to be called on behalf of another element.  0 = self.
    void setMouseDownFunction(mouseDownFunction_t f, Element *e = 0);
    void setMouseUpFunction(mouseUpFunction_t f, Element *e = 0);
    void setMouseMoveFunction(mouseMoveFunction_t f, Element *e = 0);
    void setScrollUpFunction(scrollUpFunction_t f, Element *e = 0);
    void setScrollDownFunction(scrollDownFunction_t f, Element *e = 0);

    /*
    Recurse to all children, calling _mouseDown() etc. in the lowest element that the mouse is over.
    Return value: whether this or any child has called _mouseDown().
    */
    bool onMouseDown(const Point &mousePos); 
    bool onScrollUp(const Point &mousePos);
    bool onScrollDown(const Point &mousePos);
    // Recurse to all children, calling _mouse?() in all found.
    void onMouseUp(const Point &mousePos);
    void onMouseMove(const Point &mousePos);

    void draw(); // Draw the existing texture to its designated location.
    void forceRefresh(); // Mark this and all children as changed

    static void cleanup(); // Release static resources
};

#endif