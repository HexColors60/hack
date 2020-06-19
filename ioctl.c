// This file is free software, distributed under the BSD license.

#include "hack.h"
#include "extern.h"
#include <termios.h>

struct termios termios;

void getioctls(void)
{
    tcgetattr (STDIN_FILENO, &termios);
}

void setioctls(void)
{
    tcsetattr (STDIN_FILENO, TCSADRAIN, &termios);
}
