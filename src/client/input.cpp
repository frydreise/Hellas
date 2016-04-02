// (C) 2016 Tim Gurto

#include "Client.h"
#include "Renderer.h"
#include "ui/TextBox.h"

extern Renderer renderer;

void Client::handleInput(double delta){
    static SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        std::ostringstream oss;
        switch(e.type) {
        case SDL_QUIT:
            _loop = false;
            break;

        case SDL_TEXTINPUT:
            TextBox::addText(e.text.text);
            break;

        case SDL_KEYDOWN:
            if (SDL_IsTextInputActive()) {
                // Text input

                switch(e.key.keysym.sym) {

                case SDLK_ESCAPE:
                    if (TextBox::focus() == _chatTextBox){
                        _chatTextBox->text("");
                        _chatTextBox->hide();
                    }
                    TextBox::clearFocus();
                    SDL_StopTextInput();
                    break;

                case SDLK_BACKSPACE:
                    TextBox::backspace();
                    break;

                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (TextBox::focus() == _chatTextBox){
                        SDL_StopTextInput();
                        const std::string &text = _chatTextBox->text();
                        if (text != "") {
                            if (text.at(0) == '/') {
                                // Perform command
                                performCommand(text);
                            } else {
                                performCommand("/say " + text);
                            }
                            _chatTextBox->text("");
                            _chatTextBox->hide();
                            TextBox::clearFocus();
                        }
                    }
                    break;
                }

            } else {
                // Regular key input

                switch(e.key.keysym.sym) {

                case SDLK_ESCAPE:
                {
                    if (_chatTextBox->visible()){
                        _chatTextBox->text("");
                        _chatTextBox->hide();
                        break;
                    }

                    Window *frontMostVisibleWindow = nullptr;
                    for (Window *window : _windows)
                        if (window->visible()){
                            frontMostVisibleWindow = window;
                            break;
                        }

                    if (_actionLength != 0)
                        sendMessage(CL_CANCEL_ACTION);
                    else if (frontMostVisibleWindow != nullptr)
                        frontMostVisibleWindow->hide();
                    else
                        _loop = false;
                    break;
                }

                case SDLK_SLASH:
                    if (!_chatTextBox->visible()){
                        SDL_StartTextInput();
                        _chatTextBox->show();
                        TextBox::focus(_chatTextBox);
                        _chatTextBox->text("/");
                    }
                    break;

                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (_chatTextBox->visible()){
                        // Send command/chat
                        const std::string &text = _chatTextBox->text();
                        if (text != "") {
                            if (text.at(0) == '/') {
                                // Perform command
                                performCommand(text);
                            } else {
                                performCommand("/say " + text);
                            }
                            _chatTextBox->text("");
                            _chatTextBox->hide();
                            TextBox::clearFocus();
                        }
                        break;
                    } else {
                        SDL_StartTextInput();
                        _chatTextBox->show();
                        TextBox::focus(_chatTextBox);
                    }
                    break;

                case SDLK_c:
                    _craftingWindow->toggleVisibility();
                    removeWindow(_craftingWindow);
                    addWindow(_craftingWindow);
                    break;

                case SDLK_l:
                    _chatLog->toggleVisibility();
                    break;

                case SDLK_i:
                    _inventoryWindow->toggleVisibility();
                    removeWindow(_inventoryWindow);
                    addWindow(_inventoryWindow);
                    break;

                case SDLK_r:
                    if (!_lastWhisperer.empty()) {
                        if (!SDL_IsTextInputActive())
                            SDL_StartTextInput();
                        _chatTextBox->show();
                        TextBox::focus(_chatTextBox);
                        TextBox::focus(_chatTextBox);
                    _chatTextBox->text(std::string("/whisper ") + _lastWhisperer + " ");
                    }
                    break;
                }
            }
            break;

        case SDL_MOUSEMOTION: {
            _mouse.x = e.motion.x * SCREEN_X / static_cast<double>(renderer.width());
            _mouse.y = e.motion.y * SCREEN_Y / static_cast<double>(renderer.height());
            _mouseMoved = true;
                
            Element::resetTooltip();
            for (Window *window : _windows) {
                if (Element::tooltip != nullptr)
                    break;
                if (window->visible())
                    window->onMouseMove(_mouse);
            }
            for (Element *element : _ui) {
                if (Element::tooltip != nullptr)
                    break;
                if (element->visible())
                    element->onMouseMove(_mouse);
            }

            if (!_loaded)
                break;

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
            switch (e.button.button) {
            case SDL_BUTTON_LEFT:
                _leftMouseDown = true;

                TextBox::clearFocus();

                // Send onLeftMouseDown to all visible windows
                for (Window *window : _windows)
                    if (window->visible())
                        window->onLeftMouseDown(_mouse);
                for (Element *element : _ui)
                    if (element->visible() && collision(_mouse, element->rect()))
                        element->onLeftMouseDown(_mouse);

                if (SDL_IsTextInputActive() && !TextBox::focus())
                    SDL_StopTextInput();
                else if (!SDL_IsTextInputActive() && TextBox::focus())
                    SDL_StartTextInput();

                // Bring top clicked window to front
                for (windows_t::iterator it = _windows.begin(); it != _windows.end(); ++it) {
                    Window &window = **it;
                    if (window.visible() && collision(_mouse, window.rect())) {
                        _windows.erase(it); // Invalidates iterator.
                        addWindow(&window);
                        break;
                    }
                }

                _leftMouseDownEntity = getEntityAtMouse();
                break;

            case SDL_BUTTON_RIGHT:
                // Send onRightMouseDown to all visible windows
                for (Window *window : _windows)
                    if (window->visible())
                        window->onRightMouseDown(_mouse);
                for (Element *element : _ui)
                    if (element->visible() && collision(_mouse, element->rect()))
                        element->onRightMouseDown(_mouse);

                _rightMouseDownEntity = getEntityAtMouse();
                break;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (!_loaded)
                break;

            switch (e.button.button) {
            case SDL_BUTTON_LEFT: {
                _leftMouseDown = false;

                // Construct item
                if (Container::getUseItem()) {
                    px_t
                        x = toInt(_mouse.x - offset().x),
                        y = toInt(_mouse.y - offset().y);
                    sendMessage(CL_CONSTRUCT, makeArgs(Container::useSlot, x, y));
                    prepareAction(std::string("Constructing ") +
                                  _inventory[Container::useSlot].first->name());
                    break;
                }

                bool mouseUpOnWindow = false;
                for (Window *window : _windows)
                    if (window->visible() && collision(_mouse, window->rect())) {
                        window->onLeftMouseUp(_mouse);
                        mouseUpOnWindow = true;
                        break;
                    }
                for (Element *element : _ui)
                    if (!mouseUpOnWindow &&
                        element->visible() &&
                        collision(_mouse, element->rect())) {
                        element->onLeftMouseUp(_mouse);
                        break;
                    }

                // Dragged item onto map -> drop.
                if (!mouseUpOnWindow && Container::getDragItem() != nullptr) {
                    Container::dropItem();
                }

                // Mouse down and up on same entity: onLeftClick
                if (_leftMouseDownEntity != nullptr &&
                    _currentMouseOverEntity == _leftMouseDownEntity)
                    _currentMouseOverEntity->onLeftClick(*this);
                _leftMouseDownEntity = nullptr;

                break;
            }

            case SDL_BUTTON_RIGHT:
                _rightMouseDown = false;

                // Items can only be constructed or used from the inventory, not container
                // objects.
                if (_inventoryWindow->visible()) {
                    _inventoryWindow->onRightMouseUp(_mouse);
                    const Item *useItem = Container::getUseItem();
                    if (useItem != nullptr)
                        _constructionFootprint = useItem->constructsObject()->image();
                    else
                        _constructionFootprint = Texture();
                }

                // Mouse down and up on same entity: onRightClick
                if (_rightMouseDownEntity != nullptr &&
                    _currentMouseOverEntity == _rightMouseDownEntity)
                    _currentMouseOverEntity->onRightClick(*this);
                _rightMouseDownEntity = nullptr;

                break;
            }

            break;

        case SDL_MOUSEWHEEL:
            if (e.wheel.y < 0) {
                for (Window *window : _windows)
                    if (collision(_mouse, window->rect()))
                        window->onScrollDown(_mouse);
                for (Element *element : _ui)
                    if (collision(_mouse, element->rect()))
                        element->onScrollDown(_mouse);
            } else if (e.wheel.y > 0) {
                for (Window *window : _windows)
                    if (collision(_mouse, window->rect()))
                        window->onScrollUp(_mouse);
                for (Element *element : _ui)
                    if (collision(_mouse, element->rect()))
                        element->onScrollUp(_mouse);
            }
            break;

        case SDL_WINDOWEVENT:
            switch(e.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_MAXIMIZED:
            case SDL_WINDOWEVENT_RESTORED:
                renderer.updateSize();
                renderer.setScale(static_cast<float>(renderer.width()) / SCREEN_X,
                                    static_cast<float>(renderer.height()) / SCREEN_Y);
                for (Window *window : _windows)
                    window->forceRefresh();
                for (Element *element : _ui)
                    element->forceRefresh();
                break;
            }

        default:
            // Unhandled event
            ;
        }
    }
    // Poll keys (whether they are currently pressed; not key events)
    static const Uint8 *keyboardState = SDL_GetKeyboardState(0);
    if (_loggedIn && !SDL_IsTextInputActive()) {
        bool
            up = keyboardState[SDL_SCANCODE_UP] == SDL_PRESSED ||
                    keyboardState[SDL_SCANCODE_W] == SDL_PRESSED,
            down = keyboardState[SDL_SCANCODE_DOWN] == SDL_PRESSED ||
                    keyboardState[SDL_SCANCODE_S] == SDL_PRESSED,
            left = keyboardState[SDL_SCANCODE_LEFT] == SDL_PRESSED ||
                    keyboardState[SDL_SCANCODE_A] == SDL_PRESSED,
            right = keyboardState[SDL_SCANCODE_RIGHT] == SDL_PRESSED ||
                    keyboardState[SDL_SCANCODE_D] == SDL_PRESSED;
        if (up != down || left != right) {
            static const double DIAG_SPEED = MOVEMENT_SPEED / SQRT_2;
            const double
                dist = delta * MOVEMENT_SPEED,
                diagDist = delta * DIAG_SPEED;
            Point newLoc = _pendingCharLoc;
            if (up != down) {
                if (up && !down)
                    newLoc.y -= (left != right) ? diagDist : dist;
                else if (down && !up)
                    newLoc.y += (left != right) ? diagDist : dist;
            }
            if (left && !right)
                newLoc.x -= (up != down) ? diagDist : dist;
            else if (right && !left)
                newLoc.x += (up != down) ? diagDist : dist;

            _pendingCharLoc = newLoc;
            static double const MAX_PENDING_DISTANCE = 50;
            _pendingCharLoc = interpolate(_character.location(), _pendingCharLoc,
                                            MAX_PENDING_DISTANCE);
            _mouseMoved = true;
        }
    }
}
