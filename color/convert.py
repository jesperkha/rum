# Converts json colorscheme file into highlight file readable by the editor.
# Syntax files are read at runtime so recompilation of editor is not needed.

import json

NAME_LEN = 32

# Convert hex value #xxxxxx to rgb vaue 00R;00G;00B
# 0-padding applied to have constant width for each color in file
def hex_to_rgb(value: str) -> str:
    h = value.lstrip('#')
    rgb = list(str(int(h[i:i+2], 16)) for i in (0, 2, 4))

    # Add 0-padding
    for i, col in enumerate(rgb):
        rgb[i] = ("0" * (3-len(col))) + col

    return ";".join(rgb)


def main():
    themes = []

    with open("themes.json", "r+") as f:
        theme_json = json.load(f)

        for theme in theme_json:
            colors = ""
            for v in theme_json[theme].values():
                colors += hex_to_rgb(v) + '\0'
            
            theme_name = theme + ('\0' * (NAME_LEN-len(theme)))
            themes.append(theme_name + colors)
        
    with open("themes.color", "w+") as f:
        f.writelines(themes)


if __name__ == "__main__":
    main()