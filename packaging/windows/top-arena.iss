; Installateur Windows de Top Arena (Inno Setup 6), construit par release.yml :
;   iscc /DAppVersion=X.Y.Z /DSourceDir=<dossier de windeployqt> /DOutputDir=<sortie> top-arena.iss
; Installation pour l'utilisateur seul, sans droits administrateur : le dossier reste
; accessible en écriture, ce qui permet au jeu de se mettre à jour lui-même (zip de la release).

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\dist"
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

[Setup]
; Identifiant fixe : une nouvelle version de l'installateur met à jour l'installation existante.
AppId={{D09D348D-1992-49A7-8916-25B0DF88CD23}
AppName=Top Arena
AppVersion={#AppVersion}
AppVerName=Top Arena {#AppVersion}
AppPublisher=Top Arena
AppPublisherURL=https://github.com/valentinRyckaert/Top-Arena
DefaultDirName={localappdata}\Programs\Top Arena
DefaultGroupName=Top Arena
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
SetupIconFile=..\toupie-arena.ico
UninstallDisplayIcon={app}\toupie_arena.exe
UninstallDisplayName=Top Arena
OutputDir={#OutputDir}
OutputBaseFilename=TopArena-v{#AppVersion}-windows-x64-setup
WizardStyle=modern
Compression=lzma2
SolidCompression=yes

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Top Arena"; Filename: "{app}\toupie_arena.exe"
Name: "{autodesktop}\Top Arena"; Filename: "{app}\toupie_arena.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\toupie_arena.exe"; Description: "{cm:LaunchProgram,Top Arena}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Fichiers ajoutés depuis par les mises à jour automatiques.
Type: filesandordirs; Name: "{app}"
