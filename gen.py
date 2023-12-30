# Script to convert various config files into easily readable
# files used by the editor at runtime.

# Todo: sort color names and check file format correctness

import json

CONFIG_DIR = "config"
OUTPUT_DIR = "gen"
EXTENSION = ".wim"

def gen_themes():
    NAME_LEN = 32
    themes = []

    with open(f"{CONFIG_DIR}/themes.json", "r+") as f:
        theme_json = json.load(f)

        for theme in theme_json:
            colors = ""
            for v in theme_json[theme].values():
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
        syntax_json = json.load(f)

        for extension, v in syntax_json.items():
            for ext in extension.split("/"):
                line = ext + ('\0' * (KW_LEN - len(ext)))

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