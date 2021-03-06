#include "Avatar.h"
#include "Client.h"
#include "Renderer.h"
#include "TooltipBuilder.h"
#include "../util.h"

extern Renderer renderer;

const Rect Avatar::DRAW_RECT(-9, -39, 20, 40);
const Rect Avatar::COLLISION_RECT(-5, -2, 10, 4);
std::map<std::string, EntityType> Avatar::_classes;

Avatar::Avatar(const std::string &name, const Point &location):
Entity(&_classes[""], location),
_name(name),
_gear(Client::GEAR_SLOTS, std::make_pair(nullptr, 0)),
_driving(false){}

Point Avatar::interpolatedLocation(double delta){
    if (_destination == location())
        return _destination;;

    const double maxLegalDistance = delta * Client::MOVEMENT_SPEED;
    return interpolate(location(), _destination, maxLegalDistance);
}

void Avatar::draw(const Client &client) const{
    if (_driving)
        return;

    Entity::draw(client);
    
    // Draw gear
    for (const auto &pair : ClientItem::drawOrder()){
        const ClientItem *item = _gear[pair.second].first;
        if (item != nullptr)
            item->draw(location());
    }

    if (isDebug()) {
        renderer.setDrawColor(Color::WHITE);
        renderer.drawRect(COLLISION_RECT + location() + client.offset());
    }

    // Draw username
    if (_name != client.username()) {
        const Texture outlineTexture(client.defaultFont(), _name, Color::PLAYER_NAME_OUTLINE);
        Point p = location() + client.offset();
        p.y -= 60;
        p.x -= outlineTexture.width() / 2;
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y)
                outlineTexture.draw(p + Point(x, y));
        Texture(client.defaultFont(), _name, Color::PLAYER_NAME).draw(p);
    }
}

void Avatar::update(double delta){
    location(interpolatedLocation(delta));
}

void Avatar::cleanup(){
    _classes.clear();
}

void Avatar::setClass(const std::string &c){
    _class = c;
    if (_classes.find(c) == _classes.end())
        _classes[c] = EntityType(DRAW_RECT, std::string("Images/" + c + ".png"));
    type(&_classes[c]);
}

const Texture &Avatar::tooltip() const{
    if (_tooltip)
        return _tooltip;

    // Name
    TooltipBuilder tb;
    tb.setColor(Color::ITEM_NAME);
    tb.addLine(_name);

    // Class
    tb.addGap();
    tb.setColor(Color::ITEM_TAGS);
    tb.addLine(getClass());

    // Debug info
    /*if (isDebug()){
        tb.addGap();
        tb.setColor(Color::ITEM_TAGS);
        tb.addLine("");
    }*/

    _tooltip = tb.publish();
    return _tooltip;
}
