#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc.exe
CCC=g++.exe
CXX=g++.exe
FC=gfortran.exe

# Macros
PLATFORM=MinGW-Windows

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/Release/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/alphablend.o \
	${OBJECTDIR}/init.o \
	${OBJECTDIR}/contact.o \
	${OBJECTDIR}/cluiservices.o \
	${OBJECTDIR}/Docking.o \
	${OBJECTDIR}/utf.o \
	${OBJECTDIR}/clcmsgs.o \
	${OBJECTDIR}/wallpaper.o \
	${OBJECTDIR}/forkthread.o \
	${OBJECTDIR}/clcopts.o \
	${OBJECTDIR}/clistmod.o \
	${OBJECTDIR}/clistevents.o \
	${OBJECTDIR}/clistmenus.o \
	${OBJECTDIR}/clcitems.o \
	${OBJECTDIR}/CLUIFrames/groupmenu.o \
	${OBJECTDIR}/CLUIFrames/movetogroup.o \
	${OBJECTDIR}/extBackg.o \
	${OBJECTDIR}/clcutils.o \
	${OBJECTDIR}/coolsb/coolscroll.o \
	${OBJECTDIR}/CLUIFrames/cluiframes.o \
	${OBJECTDIR}/coolsb/coolsblib.o \
	${OBJECTDIR}/clc.o \
	${OBJECTDIR}/CLCButton.o \
	${OBJECTDIR}/clcpaint.o \
	${OBJECTDIR}/CLUIFrames/framesmenu.o \
	${OBJECTDIR}/statusfloater.o \
	${OBJECTDIR}/clistopts.o \
	${OBJECTDIR}/clistsettings.o \
	${OBJECTDIR}/viewmodes.o \
	${OBJECTDIR}/clisttray.o \
	${OBJECTDIR}/cluiopts.o \
	${OBJECTDIR}/clui.o \
	${OBJECTDIR}/statusbar.o \
	${OBJECTDIR}/rowheight_funcs.o \
	${OBJECTDIR}/clcidents.o \
	${OBJECTDIR}/clnplus.o \
	${OBJECTDIR}/commonheaders.o

# C Compiler Flags
CFLAGS=-m32

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=build/resource.coff -lgdi32 -lcomctl32 -lcomdlg32 -lmsvcrt -lkernel32 -lmsimg32 -lshlwapi -luser32 -lshell32 -lshlwapi -luser32 -lole32 -lole32

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Release.mk dist/Release/${PLATFORM}/clist_nicer.dll

dist/Release/${PLATFORM}/clist_nicer.dll: ${OBJECTFILES}
	${MKDIR} -p dist/Release/${PLATFORM}
	${LINK.cc} -shared -o dist/Release/${PLATFORM}/clist_nicer.dll -s ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/alphablend.o: alphablend.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/alphablend.o alphablend.c

${OBJECTDIR}/init.o: init.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/init.o init.c

${OBJECTDIR}/contact.o: contact.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/contact.o contact.c

${OBJECTDIR}/cluiservices.o: cluiservices.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/cluiservices.o cluiservices.c

${OBJECTDIR}/Docking.o: Docking.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/Docking.o Docking.c

${OBJECTDIR}/utf.o: utf.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/utf.o utf.c

${OBJECTDIR}/clcmsgs.o: clcmsgs.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clcmsgs.o clcmsgs.c

${OBJECTDIR}/wallpaper.o: wallpaper.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/wallpaper.o wallpaper.c

${OBJECTDIR}/forkthread.o: forkthread.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/forkthread.o forkthread.c

${OBJECTDIR}/clcopts.o: clcopts.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clcopts.o clcopts.c

${OBJECTDIR}/clistmod.o: clistmod.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clistmod.o clistmod.c

${OBJECTDIR}/clistevents.o: clistevents.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clistevents.o clistevents.c

${OBJECTDIR}/clistmenus.o: clistmenus.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clistmenus.o clistmenus.c

${OBJECTDIR}/clcitems.o: clcitems.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clcitems.o clcitems.c

${OBJECTDIR}/CLUIFrames/groupmenu.o: CLUIFrames/groupmenu.c 
	${MKDIR} -p ${OBJECTDIR}/CLUIFrames
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/CLUIFrames/groupmenu.o CLUIFrames/groupmenu.c

${OBJECTDIR}/CLUIFrames/movetogroup.o: CLUIFrames/movetogroup.c 
	${MKDIR} -p ${OBJECTDIR}/CLUIFrames
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/CLUIFrames/movetogroup.o CLUIFrames/movetogroup.c

${OBJECTDIR}/extBackg.o: extBackg.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/extBackg.o extBackg.c

${OBJECTDIR}/clcutils.o: clcutils.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clcutils.o clcutils.c

${OBJECTDIR}/coolsb/coolscroll.o: coolsb/coolscroll.c 
	${MKDIR} -p ${OBJECTDIR}/coolsb
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/coolsb/coolscroll.o coolsb/coolscroll.c

${OBJECTDIR}/CLUIFrames/cluiframes.o: CLUIFrames/cluiframes.c 
	${MKDIR} -p ${OBJECTDIR}/CLUIFrames
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/CLUIFrames/cluiframes.o CLUIFrames/cluiframes.c

${OBJECTDIR}/coolsb/coolsblib.o: coolsb/coolsblib.c 
	${MKDIR} -p ${OBJECTDIR}/coolsb
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/coolsb/coolsblib.o coolsb/coolsblib.c

${OBJECTDIR}/clc.o: clc.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clc.o clc.c

${OBJECTDIR}/CLCButton.o: CLCButton.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/CLCButton.o CLCButton.c

${OBJECTDIR}/clcpaint.o: clcpaint.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clcpaint.o clcpaint.c

${OBJECTDIR}/CLUIFrames/framesmenu.o: CLUIFrames/framesmenu.c 
	${MKDIR} -p ${OBJECTDIR}/CLUIFrames
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/CLUIFrames/framesmenu.o CLUIFrames/framesmenu.c

${OBJECTDIR}/statusfloater.o: statusfloater.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/statusfloater.o statusfloater.c

${OBJECTDIR}/clistopts.o: clistopts.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clistopts.o clistopts.c

${OBJECTDIR}/clistsettings.o: clistsettings.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clistsettings.o clistsettings.c

${OBJECTDIR}/viewmodes.o: viewmodes.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/viewmodes.o viewmodes.c

${OBJECTDIR}/clisttray.o: clisttray.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clisttray.o clisttray.c

${OBJECTDIR}/cluiopts.o: cluiopts.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/cluiopts.o cluiopts.c

${OBJECTDIR}/clui.o: clui.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clui.o clui.c

${OBJECTDIR}/statusbar.o: statusbar.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/statusbar.o statusbar.c

${OBJECTDIR}/rowheight_funcs.o: rowheight_funcs.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/rowheight_funcs.o rowheight_funcs.c

${OBJECTDIR}/clcidents.o: clcidents.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clcidents.o clcidents.c

${OBJECTDIR}/clnplus.o: clnplus.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/clnplus.o clnplus.cpp

${OBJECTDIR}/commonheaders.o: commonheaders.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -s -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/commonheaders.o commonheaders.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release
	${RM} dist/Release/${PLATFORM}/clist_nicer.dll

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
