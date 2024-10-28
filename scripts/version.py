# Increments the micro version of the program

import os

FILEPATH = "include/version.h"

if __name__ == "__main__":
    text = '#define VERSION "0.0.0"'
    try:
        with open(FILEPATH, "r") as f:
            file = f.readline().removesuffix("\n")
            if file != "":
                text = file
    except:
        os.open(FILEPATH, os.O_CREAT)

    vs = text.split(" ")[2][1:-1].split(".")
    new_version = f'#define VERSION "{vs[0]}.{vs[1]}.{int(vs[2])+1}"'

    with open(FILEPATH, "w") as f:
        f.write(new_version)