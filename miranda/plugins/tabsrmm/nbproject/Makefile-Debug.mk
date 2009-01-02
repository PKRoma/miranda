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
OBJECTDIR=build/Debug/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/formatting.o \
	${OBJECTDIR}/tabmodplus/modplus.o \
	${OBJECTDIR}/containeroptions.o \
	${OBJECTDIR}/sendqueue.o \
	${OBJECTDIR}/themes.o \
	${OBJECTDIR}/ImageDataObject.o \
	${OBJECTDIR}/userprefs.o \
	${OBJECTDIR}/msgs.o \
	${OBJECTDIR}/buttonsbar.o \
	${OBJECTDIR}/typingnotify.o \
	${OBJECTDIR}/container.o \
	${OBJECTDIR}/tabctrl.o \
	${OBJECTDIR}/chat/options.o \
	${OBJECTDIR}/chat/window.o \
	${OBJECTDIR}/TSButton.o \
	${OBJECTDIR}/hotkeyhandler.o \
	${OBJECTDIR}/chat/clist.o \
	${OBJECTDIR}/chat/tools.o \
	${OBJECTDIR}/tabmodplus/msgoptions_plus.o \
	${OBJECTDIR}/trayicon.o \
	${OBJECTDIR}/templates.o \
	${OBJECTDIR}/eventpopups.o \
	${OBJECTDIR}/chat/message.o \
	${OBJECTDIR}/srmm.o \
	${OBJECTDIR}/chat/main.o \
	${OBJECTDIR}/chat/log.o \
	${OBJECTDIR}/msgoptions.o \
	${OBJECTDIR}/chat/manager.o \
	${OBJECTDIR}/generic_msghandlers.o \
	${OBJECTDIR}/msglog.o \
	${OBJECTDIR}/selectcontainer.o \
	${OBJECTDIR}/chat/services.o \
	${OBJECTDIR}/chat/colorchooser.o \
	${OBJECTDIR}/msgdialog.o \
	${OBJECTDIR}/msgdlgutils.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/${PLATFORM}/libtabsrmm.dll

dist/Debug/${PLATFORM}/libtabsrmm.dll: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/${PLATFORM}
	${LINK.cc} -shared -o dist/Debug/${PLATFORM}/libtabsrmm.dll -fPIC ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/formatting.o: formatting.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/formatting.o formatting.cpp

${OBJECTDIR}/tabmodplus/modplus.o: tabmodplus/modplus.c 
	${MKDIR} -p ${OBJECTDIR}/tabmodplus
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/tabmodplus/modplus.o tabmodplus/modplus.c

${OBJECTDIR}/containeroptions.o: containeroptions.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/containeroptions.o containeroptions.c

${OBJECTDIR}/sendqueue.o: sendqueue.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/sendqueue.o sendqueue.c

${OBJECTDIR}/themes.o: themes.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/themes.o themes.c

${OBJECTDIR}/ImageDataObject.o: ImageDataObject.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/ImageDataObject.o ImageDataObject.cpp

${OBJECTDIR}/userprefs.o: userprefs.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/userprefs.o userprefs.c

${OBJECTDIR}/msgs.o: msgs.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/msgs.o msgs.c

${OBJECTDIR}/buttonsbar.o: buttonsbar.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/buttonsbar.o buttonsbar.c

${OBJECTDIR}/typingnotify.o: typingnotify.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/typingnotify.o typingnotify.c

${OBJECTDIR}/container.o: container.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/container.o container.c

${OBJECTDIR}/tabctrl.o: tabctrl.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/tabctrl.o tabctrl.c

${OBJECTDIR}/chat/options.o: chat/options.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/options.o chat/options.c

${OBJECTDIR}/chat/window.o: chat/window.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/window.o chat/window.c

${OBJECTDIR}/TSButton.o: TSButton.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/TSButton.o TSButton.c

${OBJECTDIR}/hotkeyhandler.o: hotkeyhandler.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/hotkeyhandler.o hotkeyhandler.c

${OBJECTDIR}/chat/clist.o: chat/clist.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/clist.o chat/clist.c

${OBJECTDIR}/chat/tools.o: chat/tools.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/tools.o chat/tools.c

${OBJECTDIR}/tabmodplus/msgoptions_plus.o: tabmodplus/msgoptions_plus.c 
	${MKDIR} -p ${OBJECTDIR}/tabmodplus
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/tabmodplus/msgoptions_plus.o tabmodplus/msgoptions_plus.c

${OBJECTDIR}/trayicon.o: trayicon.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/trayicon.o trayicon.c

${OBJECTDIR}/templates.o: templates.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/templates.o templates.c

${OBJECTDIR}/eventpopups.o: eventpopups.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/eventpopups.o eventpopups.c

${OBJECTDIR}/chat/message.o: chat/message.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/message.o chat/message.c

${OBJECTDIR}/srmm.o: srmm.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/srmm.o srmm.c

${OBJECTDIR}/chat/main.o: chat/main.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/main.o chat/main.c

${OBJECTDIR}/chat/log.o: chat/log.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/log.o chat/log.c

${OBJECTDIR}/msgoptions.o: msgoptions.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/msgoptions.o msgoptions.c

${OBJECTDIR}/chat/manager.o: chat/manager.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/manager.o chat/manager.c

${OBJECTDIR}/generic_msghandlers.o: generic_msghandlers.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/generic_msghandlers.o generic_msghandlers.c

${OBJECTDIR}/msglog.o: msglog.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/msglog.o msglog.c

${OBJECTDIR}/selectcontainer.o: selectcontainer.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/selectcontainer.o selectcontainer.c

${OBJECTDIR}/chat/services.o: chat/services.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/services.o chat/services.c

${OBJECTDIR}/chat/colorchooser.o: chat/colorchooser.c 
	${MKDIR} -p ${OBJECTDIR}/chat
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/chat/colorchooser.o chat/colorchooser.c

${OBJECTDIR}/msgdialog.o: msgdialog.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/msgdialog.o msgdialog.c

${OBJECTDIR}/msgdlgutils.o: msgdlgutils.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -g -I../../include -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/msgdlgutils.o msgdlgutils.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Debug
	${RM} dist/Debug/${PLATFORM}/libtabsrmm.dll

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
