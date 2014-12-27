# Microsoft Developer Studio Project File - Name="elib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=elib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "elib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "elib.mak" CFG="elib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "elib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "elib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "elib - Win32 Release"

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

!ELSEIF  "$(CFG)" == "elib - Win32 Debug"

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

# Name "elib - Win32 Release"
# Name "elib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\conf.c
# End Source File
# Begin Source File

SOURCE=..\elist.c
# End Source File
# Begin Source File

SOURCE=..\esignal.c
# End Source File
# Begin Source File

SOURCE=..\etree.c
# End Source File
# Begin Source File

SOURCE=..\gb2uni.c
# End Source File
# Begin Source File

SOURCE=..\marshal.asm

!IF  "$(CFG)" == "elib - Win32 Release"

!ELSEIF  "$(CFG)" == "elib - Win32 Debug"

# Begin Custom Build
InputPath=..\marshal.asm

"marshal.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /c /coff /Zi ..\marshal.asm

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\memory.c
# End Source File
# Begin Source File

SOURCE=..\object.c
# End Source File
# Begin Source File

SOURCE=..\queue.c
# End Source File
# Begin Source File

SOURCE=..\std.c
# End Source File
# Begin Source File

SOURCE=..\timer.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\elib.h
# End Source File
# Begin Source File

SOURCE=..\elist.h
# End Source File
# Begin Source File

SOURCE=..\esignal.h
# End Source File
# Begin Source File

SOURCE=..\etree.h
# End Source File
# Begin Source File

SOURCE=..\list.h
# End Source File
# Begin Source File

SOURCE=..\memory.h
# End Source File
# Begin Source File

SOURCE=..\object.h
# End Source File
# Begin Source File

SOURCE=..\queue.h
# End Source File
# Begin Source File

SOURCE=..\std.h
# End Source File
# Begin Source File

SOURCE=..\timer.h
# End Source File
# Begin Source File

SOURCE=..\types.h
# End Source File
# Begin Source File

SOURCE=..\unichar.h
# End Source File
# End Group
# End Target
# End Project
