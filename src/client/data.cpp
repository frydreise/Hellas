// (C) 2016 Tim Gurto

#include "Client.h"
#include "ClientNPCType.h"
#include "../XmlReader.h"

void Client::loadData(const std::string &path){
    _terrain.clear();
    _items.clear();
    _recipes.clear();
    _objectTypes.clear();
    _objects.clear();

    // Load terrain
    XmlReader xr(path + "/terrain.xml");
    for (auto elem : xr.getChildren("terrain")) {
        int index;
        if (!xr.findAttr(elem, "index", index))
            continue;
        std::string fileName;
        if (!xr.findAttr(elem, "imageFile", fileName))
            continue;
        int isTraversable = 1;
        xr.findAttr(elem, "isTraversable", isTraversable);
        if (index >= static_cast<int>(_terrain.size()))
            _terrain.resize(index+1);
        int frames = 1, frameTime = 0;
        xr.findAttr(elem, "frames", frames);
        xr.findAttr(elem, "frameTime", frameTime);
        _terrain[index] = Terrain(fileName, isTraversable != 0, frames, frameTime);
    }

    // Load Items
    xr.newFile(path + "/items.xml");
    for (auto elem : xr.getChildren("item")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id) || !xr.findAttr(elem, "name", name))
            continue; // ID and name are mandatory.
        ClientItem item(id, name);
        std::string s;
        for (auto child : xr.getChildren("class", elem))
            if (xr.findAttr(child, "name", s)) item.addClass(s);

        if (xr.findAttr(elem, "iconFile", s))
            item.icon(s);
        else
            item.icon(id);

        if (xr.findAttr(elem, "constructs", s)){
            // Create dummy ObjectType if necessary
            auto pair = _objectTypes.insert(new ClientObjectType(s));
            item.constructsObject(*pair.first);
        }
        
        std::pair<std::set<ClientItem>::iterator, bool> ret = _items.insert(item);
        if (!ret.second) {
            ClientItem &itemInPlace = const_cast<ClientItem &>(*ret.first);
            itemInPlace = item;
        }
    }

    // Recipes
    xr.newFile(path + "/recipes.xml");
    for (auto elem : xr.getChildren("recipe")) {
        std::string id, name;
        if (!xr.findAttr(elem, "id", id))
            continue; // ID is mandatory.
        Recipe recipe(id);

        std::string s;
        if (!xr.findAttr(elem, "product", s))
            continue; // product is mandatory.
        auto it = _items.find(s);
        if (it == _items.end()) {
            _debug << Color::MMO_RED << "Skipping recipe with invalid product " << s << Log::endl;
            continue;
        }
        recipe.product(&*it);

        for (auto child : xr.getChildren("material", elem)) {
            int matQty = 1;
            xr.findAttr(child, "quantity", matQty);
            if (xr.findAttr(child, "id", s)) {
                auto it = _items.find(ClientItem(s));
                if (it == _items.end()) {
                    _debug << Color::MMO_RED << "Skipping invalid recipe material " << s << Log::endl;
                    continue;
                }
                recipe.addMaterial(&*it, matQty);
            }
        }

        for (auto child : xr.getChildren("tool", elem)) {
            if (xr.findAttr(child, "name", s)) {
                recipe.addTool(s);
            }
        }
        
        _recipes.insert(recipe);
    }

    // Object types
    xr.newFile(path + "/objectTypes.xml");
    for (auto elem : xr.getChildren("objectType")) {
        std::string s; int n;
        if (!xr.findAttr(elem, "id", s))
            continue;
        ClientObjectType *cot = new ClientObjectType(s);
        xr.findAttr(elem, "imageFile", s); // If no explicit imageFile, s will still == id
        cot->image(std::string("Images/Objects/") + s + ".png");
        if (xr.findAttr(elem, "name", s)) cot->name(s);
        Rect drawRect(0, 0, cot->width(), cot->height());
        bool
            xSet = xr.findAttr(elem, "xDrawOffset", drawRect.x),
            ySet = xr.findAttr(elem, "yDrawOffset", drawRect.y);
        if (xSet || ySet)
            cot->drawRect(drawRect);
        if (xr.getChildren("yield", elem).size() > 0) cot->canGather(true);
        if (xr.findAttr(elem, "deconstructs", s)) cot->canDeconstruct(true);
        
        auto container = xr.findChild("container", elem);
        if (container != nullptr) {
            if (xr.findAttr(container, "slots", n)) cot->containerSlots(n);
        }

        if (xr.findAttr(elem, "merchantSlots", n)) cot->merchantSlots(n);

        if (xr.findAttr(elem, "isFlat", n) && n != 0) cot->isFlat(true);
        if (xr.findAttr(elem, "gatherSound", s))
            cot->gatherSound(std::string("Sounds/") + s + ".wav");
        auto collisionRect = xr.findChild("collisionRect", elem);
        if (collisionRect) {
            Rect r;
            xr.findAttr(collisionRect, "x", r.x);
            xr.findAttr(collisionRect, "y", r.y);
            xr.findAttr(collisionRect, "w", r.w);
            xr.findAttr(collisionRect, "h", r.h);
            cot->collisionRect(r);
        }
        auto pair = _objectTypes.insert(cot);
        if (!pair.second) {
            ClientObjectType &type = const_cast<ClientObjectType &>(**pair.first);
            type = *cot;
            delete cot;
        }
    }

    // NPC types
    xr.newFile(path + "/npcTypes.xml");
    for (auto elem : xr.getChildren("npcType")) {
        std::string s;
        if (!xr.findAttr(elem, "id", s)) // No ID: skip
            continue;
        int n;
        if (!xr.findAttr(elem, "maxHealth", n)) // No max health: skip
            continue;
        ClientNPCType *nt = new ClientNPCType(s, n);
        xr.findAttr(elem, "imageFile", s); // If no explicit imageFile, s will still == id
        nt->image(std::string("Images/NPCs/") + s + ".png");
        nt->corpseImage(std::string("Images/NPCs/") + s + "-corpse.png");
        if (xr.findAttr(elem, "name", s)) nt->name(s);
        Rect drawRect(0, 0, nt->width(), nt->height());
        bool
            xSet = xr.findAttr(elem, "xDrawOffset", drawRect.x),
            ySet = xr.findAttr(elem, "yDrawOffset", drawRect.y);
        if (xSet || ySet)
            nt->drawRect(drawRect);
        auto collisionRect = xr.findChild("collisionRect", elem);
        if (collisionRect) {
            Rect r;
            xr.findAttr(collisionRect, "x", r.x);
            xr.findAttr(collisionRect, "y", r.y);
            xr.findAttr(collisionRect, "w", r.w);
            xr.findAttr(collisionRect, "h", r.h);
            nt->collisionRect(r);
        }
        auto pair = _objectTypes.insert(nt);
        if (!pair.second) {
            // A ClientObjectType is being pointed to by items; they need to point to this instead.
            const ClientObjectType *dummy = *pair.first;
            for (const ClientItem &item : _items)
                if (item.constructsObject() == dummy){
                    ClientItem &nonConstItem = const_cast<ClientItem &>(item);
                    nonConstItem.constructsObject(nt);
                }
            _objectTypes.erase(dummy);
            delete dummy;
            _objectTypes.insert(nt);
        }
    }

}
