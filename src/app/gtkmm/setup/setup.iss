; setup.iss --- Inno setup file
;
; Copyright (C) 2002, 2003 Raymond Penners <raymond@dotsphinx.com>
; All rights reserved.
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2, or (at your option)
; any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; $Id$

[Setup]
AppName=Workrave
AppVerName=Workrave 1.2.3
AppPublisher=Rob Caelers & Raymond Penners
AppPublisherURL=http://www.workrave.org
AppSupportURL=http://www.workrave.org
AppUpdatesURL=http://www.workrave.org
DefaultDirName={pf}\Workrave
DefaultGroupName=Workrave
LicenseFile=..\..\..\COPYING
AppMutex=WorkraveMutex

; uncomment the following line if you want your installation to run on NT 3.51 too.
; MinVersion=4,3.51

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional tasks:"; MinVersion: 4,4
Name: "startupmenu"; Description: "Start Workrave when Windows starts"; GroupDescription: "Additional tasks:"; MinVersion: 4,4

[Files]
Source: ".\runtime\lib\harpoon.dll"; DestDir: "{app}\lib\"; Flags: restartreplace uninsrestartdelete;
Source: ".\runtime\*.*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\images\*.png"; DestDir: "{app}\share\images\"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\sounds\*.wav"; DestDir: "{app}\share\sounds\"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\COPYING.txt"; DestDir: "{app}"; DestName: "COPYING.txt"; Flags: ignoreversion;
Source: "..\..\..\AUTHORS.txt"; DestDir: "{app}"; DestName: "AUTHORS.txt"; Flags: ignoreversion;
Source: "..\..\..\NEWS.txt"; DestDir: "{app}"; DestName: "NEWS.txt"; Flags: ignoreversion;
Source: "..\..\..\README.txt"; DestDir: "{app}"; DestName: "README.txt"; Flags: ignoreversion;
Source: "..\..\..\src\workrave.exe"; DestDir: "{app}\lib"; DestName: "Workrave.exe"; Flags: ignoreversion;
Source: "..\..\..\..\workrave-misc\doc\leaflet\leaflet-en.pdf"; DestDir: "{app}\doc"; DestName: "leaflet.pdf"; Flags: ignoreversion;
Source: "..\..\..\po\nl.gmo"; DestDir: "{app}\lib\locale\nl\LC_MESSAGES"; DestName: "workrave.mo"; Flags: ignoreversion;
Source: "..\..\..\po\de.gmo"; DestDir: "{app}\lib\locale\de\LC_MESSAGES"; DestName: "workrave.mo"; Flags: ignoreversion;
; Source: "..\..\..\po\eo.gmo"; DestDir: "{app}\lib\locale\eo\LC_MESSAGES"; DestName: "workrave.mo"; Flags: ignoreversion;

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Workrave.exe"; ValueType: string; ValueData: "{app}\lib\Workrave.exe"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Workrave.exe"; ValueName: "Path"; ValueType: string; ValueData: "{app}\lib"; Flags: uninsdeletekeyifempty

[Icons]
Name: "{group}\Workrave"; Filename: "{app}\lib\Workrave.exe"
Name: "{group}\News"; Filename: "{app}\NEWS.txt"
Name: "{group}\Read me"; Filename: "{app}\README.txt"
Name: "{group}\License"; Filename: "{app}\COPYING.txt"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{group}\Leaflet"; Filename: "{app}\doc\leaflet.pdf"
Name: "{userdesktop}\Workrave"; Filename: "{app}\lib\Workrave.exe"; MinVersion: 4,4; Tasks: desktopicon
Name: "{userstartup}\Workrave"; Filename: "{app}\lib\Workrave.exe"; MinVersion: 4,4; Tasks: startupmenu
Name: "{app}\Workrave"; Filename: "{app}\lib\Workrave.exe"


[Run]
Filename: "{app}\lib\workrave.exe"; Description: "Launch Workrave"; Flags: nowait postinstall skipifsilent


