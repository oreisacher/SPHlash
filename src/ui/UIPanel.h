//
// Created by ruut on 3/14/26.
//

#ifndef FLUIDSIMSPH_UIPANEL_H
#define FLUIDSIMSPH_UIPANEL_H

#include <functional>
#include <string>

class UIManager;

class UIPanel {
public:
    UIPanel(const std::string& name);

    virtual ~UIPanel();

    virtual void update(double deltaTime) = 0;
    virtual void draw() = 0;

    std::string getName() const;

    void close();

    void addOnCloseListener(const std::function<void()>& listener);

protected:
    friend class UIManager;
    UIManager* manager;

    bool visible = true;
    bool shouldClose = false;
    std::string name;

    std::vector<std::function<void()>> onCloseListeners;
    void notifyOnCloseListeners() const;
};

#endif //FLUIDSIMSPH_UIPANEL_H