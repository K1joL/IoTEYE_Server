#include "functional.h"

uint8_t ioteye::GetCommandCode(const std::string &cmd)
{
    if(cmd.size() == 2)
        return cmd[0] + cmd[1];
    else return 0;
}
