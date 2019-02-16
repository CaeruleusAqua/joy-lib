#include "Joystick.h"

#include <unistd.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <thread>


Joystick::Joystick(const std::string &joy_dev, double rate_limit) {
    m_period = 1000.0 * 1000.0 * (1.0 / rate_limit);
    m_joy_fd = open(joy_dev.c_str(), O_RDONLY);
    m_deadzone = 0;
    if (m_joy_fd != -1) {
        close(m_joy_fd);
        m_joy_fd = open(joy_dev.c_str(), O_RDONLY);
    }
    if (m_joy_fd < 0) {
        throw std::runtime_error(std::string("Cold not open file descriptor"));
    }
    m_wakeup_time = std::chrono::steady_clock::now() + std::chrono::microseconds(m_period);
}

Joystick::~Joystick() {
    close(m_joy_fd);
}

JoyMessage Joystick::poll() {
    double val; //Temporary variable to hold event values

    double scale = -1.0 / (1.0 - m_deadzone) / INT16_MAX;
    double unscaled_deadzone = INT16_MAX * m_deadzone;

    js_event event;
    fd_set set;

    FD_ZERO(&set);
    FD_SET(m_joy_fd, &set);

    static struct timeval tv{0, 100000}; // 100 ms timeout
    int select_out = select(m_joy_fd + 1, &set, nullptr, nullptr, &tv);

    if (select_out == -1) {
        throw std::runtime_error(std::string("Joystick disconnected"));
    }

    if (FD_ISSET(m_joy_fd, &set)) {

        if (read(m_joy_fd, &event, sizeof(js_event)) == -1 && errno != EAGAIN)
            throw std::runtime_error(std::string("Joystick disconnected"));

        switch (event.type) {
            case JS_EVENT_BUTTON:
            case JS_EVENT_BUTTON | JS_EVENT_INIT:
                if (event.number >= m_joy_msg.buttons.size()) {
                    int old_size = static_cast<int>(m_joy_msg.buttons.size());
                    m_joy_msg.buttons.resize(event.number + 1);
                    for (int i = old_size; i < m_joy_msg.buttons.size(); i++) {
                        m_joy_msg.buttons[i] = 0.0;
                    }
                }
                m_joy_msg.buttons[event.number] = (event.value ? 1 : 0);
                break;
            case JS_EVENT_AXIS:
                val = event.value;
                if (event.number >= m_joy_msg.axes.size()) {
                    int old_size = static_cast<int>(m_joy_msg.axes.size());
                    m_joy_msg.axes.resize(event.number + 1);
                    for (int i = old_size; i < m_joy_msg.axes.size(); i++) {
                        m_joy_msg.axes[i] = 0.0;
                    }
                }
                if (val > unscaled_deadzone)
                    val -= unscaled_deadzone;
                else if (val < -unscaled_deadzone)
                    val += unscaled_deadzone;
                else
                    val = 0;
                m_joy_msg.axes[event.number] = static_cast<float>(val * scale);
                break;
            default:
                break;
        }
    }
    std::this_thread::sleep_until(m_wakeup_time);
    m_wakeup_time = std::chrono::steady_clock::now() + std::chrono::microseconds(m_period);

    return m_joy_msg;
}


std::vector<std::pair<std::string, std::string>> Joystick::getJoysticks() {
    static const char path[] = "/dev/input";
    struct dirent *entry;
    struct stat stat_buf;

    std::vector<std::pair<std::string, std::string>> joysticks;

    DIR *dev_dir = opendir(path);
    if (dev_dir == nullptr) {
        printf("Couldn't open %s. Error %i: %s.\n", path, errno, strerror(errno));
        return joysticks;
    }

    while ((entry = readdir(dev_dir)) != nullptr) {
        // filter entries
        if (strncmp(entry->d_name, "js\n", 2) != 0) // skip device if it's not a joystick
            continue;

        std::string current_path = std::string(path) + "/" + entry->d_name;
        if (stat(current_path.c_str(), &stat_buf) == -1)
            continue;

        if (!S_ISCHR(stat_buf.st_mode)) // input devices are character devices, skip other
            continue;

        // get joystick name
        int joy_fd = open(current_path.c_str(), O_RDONLY);
        if (joy_fd == -1)
            continue;

        char current_joy_name[128];
        if (ioctl(joy_fd, JSIOCGNAME(sizeof(current_joy_name)), current_joy_name) < 0)
            strncpy(current_joy_name, "Unknown\n", sizeof(current_joy_name));

        close(joy_fd);
        joysticks.emplace_back(current_path, std::string(current_joy_name));
        std::cout << current_path << std::endl;
    }
    closedir(dev_dir);
    return joysticks;
}

double Joystick::getDeadzone() const {
    return m_deadzone;
}

void Joystick::setDeadzone(double deadzone) {
    m_deadzone = deadzone;
}
