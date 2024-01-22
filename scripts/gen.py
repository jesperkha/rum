# Script to convert various config files into easily readable
# files used by the editor at runtime.

import json

CONFIG_DIR = "config"
OUTPUT_DIR = "runtime"
EXTENSION  = ".wim"

def error(msg):
    print("error:", msg)
    exit(1)

def gen_themes():
    NAME_LEN = 32
    themes = []

    color_names = [
        "bg1",
        "bg0",
        "bg2",
        "fg0",
        "yellow",
        "blue",
        "pink",
        "green",
        "aqua",
        "orange",
        "red",
        "gray"
    ]

    with open(f"{CONFIG_DIR}/themes.json", "r+") as f:
        try:
            theme_json = json.load(f)
        except:
            error("failed to load theme file, invalid json format")

        for theme in theme_json:
            colors = ""
            theme_dict = sorted(theme_json[theme].items())

            if len(theme_dict) < len(color_names):
                error(f"missing colors in theme '{theme}' theme")

            for k, v in theme_dict:
                # Verify theme format
                if k not in color_names:
                    error(f"invalid color '{k}' in '{theme}' theme")

                if len(v) != 7 or v[0] != "#":
                    error(f"invalid hex value '{v}' in '{theme}' theme")

                for c in v[1:]:
                    if c not in "0123456789abcdef":
                        error(f"invalid hex value '{v}' in '{theme}' theme")

                # Convert hex value #xxxxxx to rgb vaue 00R;00G;00B
                # 0-padding applied to have constant width for each color in file
                h = v.lstrip('#')
                rgb = list(str(int(h[i:i+2], 16)) for i in (0, 2, 4))
                for i, col in enumerate(rgb):
                    rgb[i] = ("0" * (3-len(col))) + col

                colors += ";".join(rgb) + '\0'

            theme_name = theme + ('\0' * (NAME_LEN-len(theme)))
            themes.append(theme_name + colors)
        
    with open(f"{OUTPUT_DIR}/themes{EXTENSION}", "w+") as f:
        f.writelines(themes)


def gen_syntax():
    KW_LEN = 16
    syntax = []

    with open(f"{CONFIG_DIR}/syntax.json", "r+") as f:
        try:
            syntax_json = json.load(f)
        except:
            error("failed to load syntax file, invalid json format")

        for extension, v in syntax_json.items():
            for ext in extension.split("/"):
                line = ext + ('\0' * (KW_LEN - len(ext)))

                if "keywords" not in v:
                    error(f"missing 'keywords' field in '{extension}'")

                if "types" not in v:
                    error(f"missing 'types' field in '{extension}'")

                for k in v["keywords"]:
                    line += k + '\0'
                line += '?'

                for t in v["types"]:
                    line += t + '\0'
                line += '?'

                syntax.append(line + '\n')
                  
    with open(f"{OUTPUT_DIR}/syntax{EXTENSION}", "w+") as f:
        f.writelines(syntax)
          

if __name__ == "__main__":
    gen_themes()
    gen_syntax()