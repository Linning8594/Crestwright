; SPDX-License-Identifier: AGPL-3.0-or-later
; Copyright (c) 2026 Linning

#define MyAppName "Crestwright"
#define MyAppVersion "1.1.0"
#define MyAppPublisher "Linning"
#define MyAppExeName "Crestwright.exe"
#define BuildDir "D:\CheStuWorSpace_1\Crestwright\build\Crestwright_artefacts\Release"

[Setup]
AppId={{C7E9A1B4-2D63-4F58-8A17-5E3B9C0D4F26}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commonpf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=D:\CheStuWorSpace_1\Crestwright\dist
OutputBaseFilename=Crestwright_Setup_{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#BuildDir}\VST3\Crestwright.vst3\*"; DestDir: "{commonpf}\Common Files\VST3\Crestwright.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\CheStuWorSpace_1\Crestwright\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\CheStuWorSpace_1\Crestwright\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "D:\CheStuWorSpace_1\Crestwright\THIRD-PARTY-NOTICES.md"; DestDir: "{app}"; DestName: "THIRD-PARTY-NOTICES.txt"; Flags: ignoreversion

[Icons]
Name: "{group}\Crestwright (Standalone)"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\README"; Filename: "{app}\README.md"
Name: "{group}\License"; Filename: "{app}\LICENSE.txt"
Name: "{group}\Third-Party Notices"; Filename: "{app}\THIRD-PARTY-NOTICES.txt"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch Crestwright Standalone"; Flags: postinstall nowait skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{commonpf}\Common Files\VST3\Crestwright.vst3"
