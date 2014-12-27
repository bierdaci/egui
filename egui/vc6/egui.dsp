# Microsoft Developer Studio Project File - Name="egui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=egui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "egui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "egui.mak" CFG="egui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "egui - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "egui - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "egui - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "egui - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "egui - Win32 Release"
# Name "egui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\adjust.c
# End Source File
# Begin Source File

SOURCE=..\anim.c
# End Source File
# Begin Source File

SOURCE=..\bin.c
# End Source File
# Begin Source File

SOURCE=..\box.c
# End Source File
# Begin Source File

SOURCE=..\button.c
# End Source File
# Begin Source File

SOURCE=..\char.c
# End Source File
# Begin Source File

SOURCE=..\clist.c
# End Source File
# Begin Source File

SOURCE=..\ddlist.c
# End Source File
# Begin Source File

SOURCE=..\drawable.c
# End Source File
# Begin Source File

SOURCE=..\entry.c
# End Source File
# Begin Source File

SOURCE=..\event.c
# End Source File
# Begin Source File

SOURCE=..\fixed.c
# End Source File
# Begin Source File

SOURCE=..\frame.c
# End Source File
# Begin Source File

SOURCE=..\gui.c
# End Source File
# Begin Source File

SOURCE=..\hbox.c
# End Source File
# Begin Source File

SOURCE=..\hscrollbar.c
# End Source File
# Begin Source File

SOURCE=..\label.c
# End Source File
# Begin Source File

SOURCE=..\layout.c
# End Source File
# Begin Source File

SOURCE=..\menu.c
# End Source File
# Begin Source File

SOURCE=..\notebook.c
# End Source File
# Begin Source File

SOURCE=..\res.c
# End Source File
# Begin Source File

SOURCE=..\scrollbar.c
# End Source File
# Begin Source File

SOURCE=..\scrollwin.c
# End Source File
# Begin Source File

SOURCE=..\spinbtn.c
# End Source File
# Begin Source File

SOURCE=..\text.c
# End Source File
# Begin Source File

SOURCE=..\vbox.c
# End Source File
# Begin Source File

SOURCE=..\vscrollbar.c
# End Source File
# Begin Source File

SOURCE=..\widget.c
# End Source File
# Begin Source File

SOURCE=..\window.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\adjust.h
# End Source File
# Begin Source File

SOURCE=..\anim.h
# End Source File
# Begin Source File

SOURCE=..\bin.h
# End Source File
# Begin Source File

SOURCE=..\box.h
# End Source File
# Begin Source File

SOURCE=..\button.h
# End Source File
# Begin Source File

SOURCE=..\char.h
# End Source File
# Begin Source File

SOURCE=..\clist.h
# End Source File
# Begin Source File

SOURCE=..\egui.h
# End Source File
# Begin Source File

SOURCE=..\filesel.h
# End Source File
# Begin Source File

SOURCE=..\fixed.h
# End Source File
# Begin Source File

SOURCE=..\gui.h
# End Source File
# Begin Source File

SOURCE=..\label.h
# End Source File
# Begin Source File

SOURCE=..\layout.h
# End Source File
# Begin Source File

SOURCE=..\menu.h
# End Source File
# Begin Source File

SOURCE=..\notebook.h
# End Source File
# Begin Source File

SOURCE=..\text.h
# End Source File
# Begin Source File

SOURCE=..\widget.h
# End Source File
# Begin Source File

SOURCE=..\window.h
# End Source File
# End Group
# End Target
# End Project
