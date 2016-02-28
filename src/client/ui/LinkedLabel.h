// (C) 2016 Tim Gurto

#ifndef LINKED_LABEL
#define LINKED_LABEL

#include <sstream>
#include <string>
#include "Label.h"

// Contains a Label, but its contents are linked to a variable.
template <typename T>
class LinkedLabel : public Label{
    const T &_val;
    T _lastCheckedVal;
    std::string _prefix, _suffix;

public:
    LinkedLabel(const Rect &rect, const T &val, const std::string &prefix = "",
                const std::string &suffix = "", Justification justificationH = LEFT_JUSTIFIED,
                Justification justificationV = TOP_JUSTIFIED);

private:
    virtual void checkIfChanged() override;
};

template<typename T>
LinkedLabel<T>::LinkedLabel(const Rect &rect, const T &val, const std::string &prefix,
                            const std::string &suffix, Justification justificationH,
                            Justification justificationV):
Label(rect, "", justificationH, justificationV),
_val(val),
_lastCheckedVal(val),
_prefix(prefix),
_suffix(suffix)
{}

template<typename T>
void LinkedLabel<T>::checkIfChanged(){
    if (_val != _lastCheckedVal) {
        _lastCheckedVal = _val;

        // Update text
        std::ostringstream oss;
        if (!_prefix.empty())
            oss << _prefix;
        oss << _val;
        if (!_suffix.empty())
            oss << _suffix;
        _text = oss.str();

        markChanged();
        Label::checkIfChanged();
    }
}

#endif