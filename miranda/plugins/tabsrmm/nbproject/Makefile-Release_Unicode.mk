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
OBJECTDIR=build/Release_Unicode/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/formatting.o \
	${OBJECTDIR}/tabmodplus/modplus.o \
	${OBJECTDIR}/src/containeroptions.o \
	${OBJECTDIR}/src/sendqueue.o \
	${OBJECTDIR}/src/themes.o \
	${OBJECTDIR}/src/ImageDataObject.o \
	${OBJECTDIR}/src/userprefs.o \
	${OBJECTDIR}/src/msgs.o \
	${OBJECTDIR}/src/buttonsbar.o \
	${OBJECTDIR}/src/typingnotify.o \
	${OBJECTDIR}/src/container.o \
	${OBJECTDIR}/src/tabctrl.o \
	${OBJECTDIR}/chat/window.o \
	${OBJECTDIR}/chat/options.o \
	${OBJECTDIR}/src/TSButton.o \
	${OBJECTDIR}/src/hotkeyhandler.o \
	${OBJECTDIR}/chat/clist.o \
	${OBJECTDIR}/chat/tools.o \
	${OBJECTDIR}/tabmodplus/msgoptions_plus.o \
	${OBJECTDIR}/src/trayicon.o \
	${OBJECTDIR}/src/templates.o \
	${OBJECTDIR}/src/eventpopups.o \
	${OBJECTDIR}/chat/message.o \
	${OBJECTDIR}/src/srmm.o \
	${OBJECTDIR}/chat/main.o \
	${OBJECTDIR}/chat/log.o \
	${OBJECTDIR}/src/msgoptions.o \
	${OBJECTDIR}/chat/manager.o \
	${OBJECTDIR}/src/generic_msghandlers.o \
	${OBJECTDIR}/src/msglog.o \
	${OBJECTDIR}/src/selectcontainer.o \
	${OBJECTDIR}/chat/services.o \
	${OBJECTDIR}/chat/colorchooser.o \
	${OBJECTDIR}/src/msgdialog.o \
	${OBJECTDIR}/src/msgdlgutils.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=build/tabsrmm_privateW.coff -lkernel32 -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lole32 -lshell32 -lshlwapi -luuid

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Release_Unicode.mk dist/Release_Unicode/${PLATFORM}/tabsrmm.dll

dist/Release_Unicode/${PLATFORM}/tabsrmm.dll: ${OBJECTFILES}
	${MKDIR} -p dist/Release_Unicode/${PLATFORM}
	${LINK.cc} -shared -o dist/Release_Unicode/${PLATFORM}/tabsrmm.dll -s ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/src/formatting.o: src/formatting.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/formatting.o src/formatting.cpp

${OBJECTDIR}/tabmodplus/modplus.o: tabmodplus/modplus.cpp
	${MKDIR} -p ${OBJECTDIR}/tabmodplus
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/tabmodplus/modplus.o tabmodplus/modplus.cpp

${OBJECTDIR}/src/containeroptions.o: src/containeroptions.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/containeroptions.o src/containeroptions.cpp

${OBJECTDIR}/src/sendqueue.o: src/sendqueue.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/sendqueue.o src/sendqueue.cpp

${OBJECTDIR}/src/themes.o: src/themes.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/themes.o src/themes.cpp

${OBJECTDIR}/src/ImageDataObject.o: src/ImageDataObject.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/ImageDataObject.o src/ImageDataObject.cpp

${OBJECTDIR}/src/userprefs.o: src/userprefs.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/userprefs.o src/userprefs.cpp

${OBJECTDIR}/src/msgs.o: src/msgs.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/msgs.o src/msgs.cpp

${OBJECTDIR}/src/buttonsbar.o: src/buttonsbar.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/buttonsbar.o src/buttonsbar.cpp

${OBJECTDIR}/src/typingnotify.o: src/typingnotify.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/typingnotify.o src/typingnotify.cpp

${OBJECTDIR}/src/container.o: src/container.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/container.o src/container.cpp

${OBJECTDIR}/src/tabctrl.o: src/tabctrl.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/tabctrl.o src/tabctrl.cpp

${OBJECTDIR}/chat/window.o: chat/window.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/window.o chat/window.cpp

${OBJECTDIR}/chat/options.o: chat/options.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/options.o chat/options.cpp

${OBJECTDIR}/src/TSButton.o: src/TSButton.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/TSButton.o src/TSButton.cpp

${OBJECTDIR}/src/hotkeyhandler.o: src/hotkeyhandler.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/hotkeyhandler.o src/hotkeyhandler.cpp

${OBJECTDIR}/chat/clist.o: chat/clist.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/clist.o chat/clist.cpp

${OBJECTDIR}/chat/tools.o: chat/tools.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/tools.o chat/tools.cpp

${OBJECTDIR}/tabmodplus/msgoptions_plus.o: tabmodplus/msgoptions_plus.cpp
	${MKDIR} -p ${OBJECTDIR}/tabmodplus
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/tabmodplus/msgoptions_plus.o tabmodplus/msgoptions_plus.cpp

${OBJECTDIR}/src/trayicon.o: src/trayicon.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/trayicon.o src/trayicon.cpp

${OBJECTDIR}/src/templates.o: src/templates.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/templates.o src/templates.cpp

${OBJECTDIR}/src/eventpopups.o: src/eventpopups.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/eventpopups.o src/eventpopups.cpp

${OBJECTDIR}/chat/message.o: chat/message.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/message.o chat/message.cpp

${OBJECTDIR}/src/srmm.o: src/srmm.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/srmm.o src/srmm.cpp

${OBJECTDIR}/chat/main.o: chat/main.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/main.o chat/main.cpp

${OBJECTDIR}/chat/log.o: chat/log.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/log.o chat/log.cpp

${OBJECTDIR}/src/msgoptions.o: src/msgoptions.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/msgoptions.o src/msgoptions.cpp

${OBJECTDIR}/chat/manager.o: chat/manager.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/manager.o chat/manager.cpp

${OBJECTDIR}/src/generic_msghandlers.o: src/generic_msghandlers.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/generic_msghandlers.o src/generic_msghandlers.cpp

${OBJECTDIR}/src/msglog.o: src/msglog.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/msglog.o src/msglog.cpp

${OBJECTDIR}/src/selectcontainer.o: src/selectcontainer.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/selectcontainer.o src/selectcontainer.cpp

${OBJECTDIR}/chat/services.o: chat/services.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/services.o chat/services.cpp

${OBJECTDIR}/chat/colorchooser.o: chat/colorchooser.cpp
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/colorchooser.o chat/colorchooser.cpp

${OBJECTDIR}/src/msgdialog.o: src/msgdialog.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/msgdialog.o src/msgdialog.cpp

${OBJECTDIR}/src/msgdlgutils.o: src/msgdlgutils.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} $@.d
	$(COMPILE.cc) -Wno-write-strings -O3 -s -DUNICODE -D_UNICODE -D__GNUWIN32__ -I../../include -I../../include/mingw -MMD -MP -MF $@.d -o ${OBJECTDIR}/src/msgdlgutils.o src/msgdlgutils.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release_Unicode
	${RM} dist/Release_Unicode/${PLATFORM}/tabsrmm.dll

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
