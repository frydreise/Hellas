// (C) 2015 Tim Gurto

#include "Picture.h"
#include "Renderer.h"

extern Renderer renderer;

Picture::Picture(const SDL_Rect &rect, const Texture &srcTexture):
Element(rect),
_srcTexture(srcTexture){}

void Picture::refresh(){
    renderer.pushRenderTarget(_texture);

    _srcTexture.draw(makeRect(0, 0, rect().w, rect().h));

    drawChildren();

    renderer.popRenderTarget();
}
