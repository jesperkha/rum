# Script to convert various config files into easily readable
# files used by the editor at runtime.

import json
import re
import struct # https://docs.python.org/3/library/struct.html#format-characters

CONFIG_DIR = "config"
OUTPUT_DIR = "runtime"
EXTENSION  = ".wim"


def error(msg):
    print("error:", msg)
    exit(1)

def hex_to_rgb(h: str) -> str:
    # Convert hex value #xxxxxx to rgb vaue 00R;00G;00B
    # 0-padding applied to have constant width for each color in file
    h = h.lstrip('#')
    rgb = list(str(int(h[i:i+2], 16)) for i in (0, 2, 4))
    for i, col in enumerate(rgb):
        rgb[i] = ("0" * (3-len(col))) + col
    return ";".join(rgb)

def pack_themes():
    themes = []
    # Must be same order as Colors struct in src/editor/objects.h
    color_names = [
        "bg0", "bg1", "bg2", "fg0", "aqua", "blue",
        "gray", "pink", "green", "orange", "red", "yellow"
    ]
    with open(f"{CONFIG_DIR}/themes.json", "r") as f:
        try:
            theme_json = json.load(f)
        except:
            error("themes.json: failed to load file, invalid json format")

        for name, theme in theme_json.items():
            if sorted(color_names) != sorted(theme.keys()):
                error(f"themes.json: colors do not match format in '{name}'")
            
            for col_name, col_value in theme.items():
                if not re.search(r'^#(?:[0-9a-fA-F]{3}){1,2}$', col_value):
                    error(f"themes.json: invalid hex value for '{col_name}' in '{name}'")
            
            colors = []
            for col_name in color_names:
                color = hex_to_rgb(theme[col_name])
                colors.append(color.encode())

            fmt = "32s" + "11sx"*len(color_names)
            byt = struct.pack(fmt, name.encode(), *colors)
            themes.append(byt)

    with open(f"{OUTPUT_DIR}/themes{EXTENSION}", "wb") as out:
        out.writelines(themes)

def pack_config():
    # see Config in src/editor/objects.h
    CONFIG_FORMAT = "???B"
    config_struct = {
        "syntaxEnabled": bool,
        "matchParen": bool,
        "useCRLF": bool,
        "tabSize": int,
    }

    with open(f"{CONFIG_DIR}/config.json", "r") as f:
        try:
            config_json = json.load(f)
        except:
            error("config.json: failed to load file, invalid json format")
        
        for k, v in config_json.items():
            if k not in config_struct:
                error(f"config.json: invalid field '{k}' in config.json")
            if type(v) != config_struct[k]:
                error(f"config.json: expected '{k}' to be {config_struct[k].__name__}, got {type(v).__name__}")
            if type(v) == int and v < 0:
                error(f"config.json: value for '{k}' must be between 0 and 255")
            
        with open(f"{OUTPUT_DIR}/config.wim", "wb") as out:
            out.write(struct.pack(CONFIG_FORMAT, *config_json.values()))

def pack_syntax():
    KW_LEN = 16
    syntax = []

    with open(f"{CONFIG_DIR}/syntax.json", "r") as f:
        try:
            syntax_json = json.load(f)
        except:
            error("syntax.json: failed to load file, invalid json format")

        for extension, v in syntax_json.items():
            for ext in extension.split("/"):
                line = ext + ('\0' * (KW_LEN - len(ext)))

                if "keywords" not in v:
                    error(f"syntax.json: missing 'keywords' field in '{extension}'")

                if "types" not in v:
                    error(f"syntax.json: missing 'types' field in '{extension}'")

                for k in v["keywords"]:
                    line += k + '\0'
                line += '?'

                for t in v["types"]:
                    line += t + '\0'
                line += '?'

                syntax.append(line + '\n')
                  
    with open(f"{OUTPUT_DIR}/syntax{EXTENSION}", "w") as f:
        f.writelines(syntax)
          

if __name__ == "__main__":
    pack_themes()
    pack_config()
    pack_syntax()