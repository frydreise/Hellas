// (C) 2016 Tim Gurto

#ifndef ITEM_H
#define ITEM_H

#include <set>
#include <string>
#include <vector>

class Item{
protected:
    std::string _id; // The no-space, unique name used in data files
    std::set<std::string> _classes;

public:
    Item(const std::string &id);
    virtual ~Item(){}

    const std::string &id() const { return _id; }
    const std::set<std::string> &classes() const { return _classes; }
    
    bool operator<(const Item &rhs) const { return _id < rhs._id; }
    
    typedef std::vector<std::pair<const Item *, size_t> > vect_t;

    void addClass(const std::string &className);
    bool hasClasses() const { return _classes.size() > 0; }
    bool isClass(const std::string &className) const;
};

#endif