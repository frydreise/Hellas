#include "Tree.h"
#include "Client.h"
#include "Color.h"
#include "Server.h"
#include "util.h"

EntityType Tree::_entityType(makeRect(-20, -47));

Tree::Tree(const Tree &rhs):
Entity(rhs),
_serial(rhs._serial){}

Tree::Tree(size_t serialArg, const Point &loc):
Entity(_entityType, loc),
_serial(serialArg){}

void Tree::onLeftClick(Client &client) const{
    std::ostringstream oss;
    oss << '[' << CL_COLLECT_TREE << ',' << _serial << ']';
    client.socket().sendMessage(oss.str());
    client.setAction("Gathering tree", TreeLite::ACTION_TIME);
}

std::vector<std::string> Tree::getTooltipMessages(const Client &client) const {
    std::vector<std::string> text;
    text.push_back("Tree");
    if (distance(location(), client.character().location()) > Server::ACTION_DISTANCE)
        text.push_back("Out of range");
    return text;
}
