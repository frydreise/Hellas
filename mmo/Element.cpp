// (C) 2015 Tim Gurto

#include "Element.h"

extern Renderer renderer;

const Color Element::BACKGROUND_COLOR = Color::GREY_4;
const Color Element::SHADOW_LIGHT = Color::GREY_2;
const Color Element::SHADOW_DARK = Color::GREY_8;
const Color Element::FONT_COLOR = Color::WHITE;
TTF_Font *Element::_font = 0;

Texture Element::transparentBackground;

const Point *Element::absMouse = 0;

Element::Element(const SDL_Rect &rect):
_rect(rect),
_texture(rect.w, rect.h),
_changed(true),
_mouseDown(0), _mouseDownElement(0),
_mouseUp(0), _mouseUpElement(0),
_mouseMove(0), _mouseMoveElement(0),
_parent(0){
    _texture.setBlend(SDL_BLENDMODE_BLEND);
}

Element::~Element(){
    // Free children
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        delete *it;
}

void Element::markChanged(){
    _changed = true;
    if (_parent)
        _parent->markChanged();
}

void Element::drawChildren() const{
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
        if ((*it)->_changed)
            _changed = true;

        (*it)->draw();
    }
}

void Element::checkIfChanged(){
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->checkIfChanged();
}

bool Element::onMouseDown(const Point &mousePos){
    // Assumption: if this is called, then the mouse collides with the element.
    // Assumption: each element has at most one child that collides with the mouse.
    const Point relativeLocation = mousePos - location();
    bool functionCalled = false;
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it) {
        if (collision(relativeLocation, (*it)->rect())) {
            if ((*it)->onMouseDown(relativeLocation)) {
                functionCalled = true;
            }
        }
    }
    if (functionCalled)
        return true;
    /*
    If execution gets here, then this element has no children that both collide
    and have _mouseDown defined.
    */
    if (_mouseDown) {
        _mouseDown(*_mouseDownElement);
        return true;
    } else
        return false;
}

void Element::onMouseUp(const Point &mousePos){
    const Point relativeLocation = mousePos - location();
    if (_mouseUp)
        _mouseUp(*_mouseUpElement, relativeLocation);
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->onMouseUp(relativeLocation);
}

void Element::onMouseMove(const Point &mousePos){
    const Point relativeLocation = mousePos - location();
    if (_mouseMove)
        _mouseMove(*_mouseMoveElement, relativeLocation);
    for (std::list<Element*>::const_iterator it = _children.begin(); it != _children.end(); ++it)
        (*it)->onMouseMove(relativeLocation);
}

void Element::setMouseDownFunction(mouseDownFunction_t f, Element *e){
    _mouseDown = f;
    _mouseDownElement = e ? e : this;
}

void Element::setMouseUpFunction(mouseUpFunction_t f, Element *e){
    _mouseUp = f;
    _mouseUpElement = e ? e : this;
}

void Element::setMouseMoveFunction(mouseMoveFunction_t f, Element *e){
    _mouseMove = f;
    _mouseMoveElement = e ? e : this;
}

void Element::addChild(Element *child){
    _children.push_back(child);
    child->_parent = this;
    markChanged();
};

void Element::refresh(){
    renderer.pushRenderTarget(_texture);
    drawChildren();
    renderer.popRenderTarget();
}

void Element::draw(){
    if (!_parent)
        checkIfChanged();
    if (_changed) {
        refresh();
        _changed = false;
    }
    _texture.draw(_rect);
}

void Element::makeBackgroundTransparent(){
    if (!transparentBackground) {
        transparentBackground = Texture(renderer.width(), renderer.height());
        transparentBackground.setAlpha(0);
        transparentBackground.setBlend(SDL_BLENDMODE_NONE);
        renderer.pushRenderTarget(transparentBackground);
        renderer.fill();
        renderer.popRenderTarget();
    }
    SDL_Rect rect = makeRect(0, 0,_rect.w, _rect.h);
    transparentBackground.draw(rect, rect);
}

void Element::cleanup(){
    transparentBackground = Texture();
}
