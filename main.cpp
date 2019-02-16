#include "Joystick.h"
#include <unistd.h>


int main(int argc, char **argv) {
    while (true) {
        try {
            Joystick j("/dev/input/js0");
            while (true) {
                JoyMessage msg;
                msg = j.poll();
                msg.print();
            }
        } catch (std::runtime_error &e) {
            std::cout << e.what() << std::endl;
            std::cout << "Try reconnect!" << std::endl;
        }
        sleep(1);
    }
    return 0;
}