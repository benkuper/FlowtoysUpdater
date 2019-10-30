; -- Example1.iss --
; Demonstrates copying 3 files and creating an icon.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

#define ApplicationName 'FlowtoysUpdater'
#define ApplicationVersion GetStringFileInfo('Binaries/CI/App/FlowtoysUpdater.exe',"ProductVersion")
                             
[Setup]
AppName={#ApplicationName}
AppId={#ApplicationName}
AppVersion={#ApplicationVersion}
AppPublisher=Ben Kuper
AppPublisherURL=http://www.flowtoys.com
DefaultDirName={pf}\{#ApplicationName}
DefaultGroupName={#ApplicationName}
UninstallDisplayIcon={app}\{#ApplicationName}.exe
UninstallDisplayName={#ApplicationName}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
OutputDir=/
OutputBaseFilename={#ApplicationName}-win-x64
SetupIconFile=setup.ico
 

[Messages]
SetupWindowTitle={#ApplicationName} {#ApplicationVersion} Setup

[Files]
Source: "Binaries/CI/App/{#ApplicationName}.exe"; DestDir: "{app}"
Source: "Binaries/CI/App/*.dll"; DestDir: "{app}"
Source: "redist\vcredist_x64.exe"; DestDir: "{tmp}";

[Icons]
Name: "{group}\{#ApplicationName}"; Filename: "{app}\{#ApplicationName}.exe"

[Run]
Filename: "{app}\{#ApplicationName}.exe"; Description: "{cm:LaunchProgram,{#ApplicationName}.exe}"; Flags: nowait postinstall skipifsilent
Filename: "{tmp}\vcredist_x64.exe"; Parameters: "/install /passive /norestart"; Check: not VCinstalled64

[Code]
function GetUninstallString(): String;
var
  sUnInstPath: String;
  sUnInstallString: String;
begin
  sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\{#emit SetupSetting("AppId")}_is1');
  sUnInstallString := '';
  if not RegQueryStringValue(HKLM, sUnInstPath, 'UninstallString', sUnInstallString) then
    RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;


/////////////////////////////////////////////////////////////////////
function IsUpgrade(): Boolean;
begin
  Result := (GetUninstallString() <> '');
end;


/////////////////////////////////////////////////////////////////////
function UnInstallOldVersion(): Integer;
var
  sUnInstallString: String;
  iResultCode: Integer;
begin
// Return Values:
// 1 - uninstall string is empty
// 2 - error executing the UnInstallString
// 3 - successfully executed the UnInstallString

  // default return value
  Result := 0;

  // get the uninstall string of the old app
  sUnInstallString := GetUninstallString();
  if sUnInstallString <> '' then begin
    sUnInstallString := RemoveQuotes(sUnInstallString);
    if Exec(sUnInstallString, '/SILENT /NORESTART /SUPPRESSMSGBOXES','', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
      Result := 3
    else
      Result := 2;
  end else
    Result := 1;
end;

/////////////////////////////////////////////////////////////////////
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep=ssInstall) then
  begin
    if (IsUpgrade()) then
    begin
      UnInstallOldVersion();
    end;
  end;
end;


//////////////////////////////////////////////////////////////////////
function VCinstalled64: Boolean;
var
installed: Cardinal;
key: String;
begin
  Result := False;
  key := 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64';
  //if DirExists 
 //('Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64') 
  //  then begin 
      if RegQueryDWordValue(HKEY_LOCAL_MACHINE, key, 'Installed', installed) 
      then begin
          Log('VC is installed ? ' + IntToStr(installed));  
          if installed = 1 then begin
            Result := True;
          end;
      end;
  //end    
  //else begin
  //  Log('VC directory not found ');
 // end;
 end;