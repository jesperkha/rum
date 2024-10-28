import os

def log(msg):
    print(f"[LOG]: {msg}")

def panic(msg):
    print(f"[PANIC]: {msg}")
    exit(1)

VERSION = input("Enter release version as x.x.x: ")

inno_setup = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
inno_flags = f"/Q /DVERSION={VERSION}"
script_path = "scripts/installer.iss"

log("Writing version to header")
with open("include/version.h", "w") as f:
    f.write(f'#define VERSION "{VERSION}"')

log("Building release binary")
if os.system("make -j -s release") != 0:
    panic("Failed to build rum")

log("Compiling setup")
if os.system(f'"{inno_setup}" {inno_flags} {script_path}') != 0:
    panic("Failed to compile setup")

log("Setup created in /dist")