# Increments the micro version of the program

import os

if __name__ == "__main__":
    text = '#define VERSION "0.0.0"'
    try:
        with open("src/version.h", "r") as f:
            file = f.read()
            if file != "":
                text = file
    except:
        os.open("src/version.h", os.O_CREAT)

    vs = text.split(" ")[2][1:-1].split(".")
    new_version = f'#define VERSION "{vs[0]}.{vs[1]}.{int(vs[2])+1}"'

    with open("src/version.h", "w") as f:
        f.write(new_version)