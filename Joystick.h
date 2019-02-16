#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <chrono>


class JoyMessage {
public:
    std::vector<float> axes;
    std::vector<float> buttons;

    void print() {
        std::cout << "Axes:\n";
        for (auto &axe : axes) {
            std::cout << "\t" << axe << " ";
        }
        std::cout << "\n";
        std::cout << "Buttons:\n";
        for (auto &button : buttons) {
            std::cout << "\t" << button << " ";
        }
        std::cout << std::endl;
    }
};

class Joystick {
public:
    Joystick(const std::string &joy_dev, double rate_limit = 100.0);

    ~Joystick();

    JoyMessage poll();

    double getDeadzone() const;

    void setDeadzone(double deadzone);

    /*! \brief Returns the device path and name of all joysticks.*/
    static std::vector<std::pair<std::string, std::string>> getJoysticks();


private:
    double m_deadzone;
    int m_joy_fd;
    JoyMessage m_joy_msg;
    int64_t m_period;
    std::chrono::steady_clock::time_point m_wakeup_time;

};



