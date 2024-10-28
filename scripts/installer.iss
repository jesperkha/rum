; Installs to C:\Program Files\rum[version]
; Adds installation dir to PATH

#define PROJECT_ROOT ".."

[Setup]
AppName=rum
AppVersion={#VERSION}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
DefaultDirName={commonpf}\rum{#VERSION}
OutputBaseFilename=RumInstaller
OutputDir={#PROJECT_ROOT}\dist
Compression=lzma
SolidCompression=yes
ChangesEnvironment=yes

AppPublisher=Jesper Hammer
AppPublisherURL=https://github.com/jesperkha
AppId=AAB034DF-146B-4B21-BBCB-6867CC023801

[Files]
Source: "{#PROJECT_ROOT}\rum.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PROJECT_ROOT}\config\*"; DestDir: "{app}\config"; Flags: recursesubdirs createallsubdirs
Source: "{#PROJECT_ROOT}\README.md"; DestDir: "{app}"; Flags: ignoreversion

; Add to PATH
[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; \
  ValueName: "Path"; ValueData: "{olddata};{app}"; Flags: preservestringtype

[Run]
Filename: "{app}\rum.exe"; Description: "Launch rum"; Flags: nowait postinstall skipifsilent