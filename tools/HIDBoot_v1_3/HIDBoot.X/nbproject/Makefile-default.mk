#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
include Makefile

# Environment
MKDIR=mkdir -p
RM=rm -f 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/HIDBoot.X.${IMAGE_TYPE}.elf
else
IMAGE_TYPE=production
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/HIDBoot.X.${IMAGE_TYPE}.elf
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1360907413/usb_device.o ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o ${OBJECTDIR}/_ext/1472/main.o ${OBJECTDIR}/_ext/1472/usb_descriptors.o


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

# Path to java used to run MPLAB X when this makefile was created
MP_JAVA_PATH=C:\\Program\ Files\\Java\\jre6/bin/
OS_CURRENT="$(shell uname -s)"
############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
MP_CC=C:\\Embedded\\Microchip\\MPLAB\ C32\ Suite\\bin\\pic32-gcc.exe
# MP_BC is not defined
MP_AS=C:\\Embedded\\Microchip\\MPLAB\ C32\ Suite\\bin\\pic32-as.exe
MP_LD=C:\\Embedded\\Microchip\\MPLAB\ C32\ Suite\\bin\\pic32-ld.exe
MP_AR=C:\\Embedded\\Microchip\\MPLAB\ C32\ Suite\\bin\\pic32-ar.exe
# MP_BC is not defined
MP_CC_DIR=C:\\Embedded\\Microchip\\MPLAB\ C32\ Suite\\bin
# MP_BC_DIR is not defined
MP_AS_DIR=C:\\Embedded\\Microchip\\MPLAB\ C32\ Suite\\bin
MP_LD_DIR=C:\\Embedded\\Microchip\\MPLAB\ C32\ Suite\\bin
MP_AR_DIR=C:\\Embedded\\Microchip\\MPLAB\ C32\ Suite\\bin
# MP_BC_DIR is not defined

.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/HIDBoot.X.${IMAGE_TYPE}.elf

MP_PROCESSOR_OPTION=32MX695F512H
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1472/main.o: ../main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.ok ${OBJECTDIR}/_ext/1472/main.o.err 
	@echo ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1472/main.o.d -o ${OBJECTDIR}/_ext/1472/main.o ../main.c  
	@-${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1472/main.o.d -o ${OBJECTDIR}/_ext/1472/main.o ../main.c   2>&1  > ${OBJECTDIR}/_ext/1472/main.o.err ; if [ $$? -eq 0 ] ; then touch ${OBJECTDIR}/_ext/1472/main.o.ok ; fi 
	@touch ${OBJECTDIR}/_ext/1472/main.o.d 
	
ifneq (,$(findstring MINGW32,$(OS_CURRENT))) 
	@sed -e 's/\\$$/__EOL__/g' -e 's/\\ /__ESCAPED_SPACES__/g' -e 's/\\/\//g' -e 's/__ESCAPED_SPACES__/\\ /g' -e 's/__EOL__$$/\\/g' ${OBJECTDIR}/_ext/1472/main.o.d > ${OBJECTDIR}/_ext/1472/main.o.tmp
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${CP} ${OBJECTDIR}/_ext/1472/main.o.tmp ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.tmp
endif
	@touch ${OBJECTDIR}/_ext/1472/main.o.err 
	@cat ${OBJECTDIR}/_ext/1472/main.o.err 
	@if [ -f ${OBJECTDIR}/_ext/1472/main.o.ok ] ; then rm -f ${OBJECTDIR}/_ext/1472/main.o.ok; else exit 1; fi
	
${OBJECTDIR}/_ext/1472/usb_descriptors.o: ../usb_descriptors.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.ok ${OBJECTDIR}/_ext/1472/usb_descriptors.o.err 
	@echo ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d -o ${OBJECTDIR}/_ext/1472/usb_descriptors.o ../usb_descriptors.c  
	@-${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d -o ${OBJECTDIR}/_ext/1472/usb_descriptors.o ../usb_descriptors.c   2>&1  > ${OBJECTDIR}/_ext/1472/usb_descriptors.o.err ; if [ $$? -eq 0 ] ; then touch ${OBJECTDIR}/_ext/1472/usb_descriptors.o.ok ; fi 
	@touch ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d 
	
ifneq (,$(findstring MINGW32,$(OS_CURRENT))) 
	@sed -e 's/\\$$/__EOL__/g' -e 's/\\ /__ESCAPED_SPACES__/g' -e 's/\\/\//g' -e 's/__ESCAPED_SPACES__/\\ /g' -e 's/__EOL__$$/\\/g' ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d > ${OBJECTDIR}/_ext/1472/usb_descriptors.o.tmp
	@${RM} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d 
	@${CP} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.tmp ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.tmp
endif
	@touch ${OBJECTDIR}/_ext/1472/usb_descriptors.o.err 
	@cat ${OBJECTDIR}/_ext/1472/usb_descriptors.o.err 
	@if [ -f ${OBJECTDIR}/_ext/1472/usb_descriptors.o.ok ] ; then rm -f ${OBJECTDIR}/_ext/1472/usb_descriptors.o.ok; else exit 1; fi
	
${OBJECTDIR}/_ext/1360907413/usb_function_hid.o: ../USB/usb_function_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1360907413 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.ok ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.err 
	@echo ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d -o ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o ../USB/usb_function_hid.c  
	@-${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d -o ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o ../USB/usb_function_hid.c   2>&1  > ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.err ; if [ $$? -eq 0 ] ; then touch ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.ok ; fi 
	@touch ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d 
	
ifneq (,$(findstring MINGW32,$(OS_CURRENT))) 
	@sed -e 's/\\$$/__EOL__/g' -e 's/\\ /__ESCAPED_SPACES__/g' -e 's/\\/\//g' -e 's/__ESCAPED_SPACES__/\\ /g' -e 's/__EOL__$$/\\/g' ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d > ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.tmp
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d 
	@${CP} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.tmp ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.tmp
endif
	@touch ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.err 
	@cat ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.err 
	@if [ -f ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.ok ] ; then rm -f ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.ok; else exit 1; fi
	
${OBJECTDIR}/_ext/1360907413/usb_device.o: ../USB/usb_device.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1360907413 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_device.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_device.o.ok ${OBJECTDIR}/_ext/1360907413/usb_device.o.err 
	@echo ${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1360907413/usb_device.o.d -o ${OBJECTDIR}/_ext/1360907413/usb_device.o ../USB/usb_device.c  
	@-${MP_CC} $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1360907413/usb_device.o.d -o ${OBJECTDIR}/_ext/1360907413/usb_device.o ../USB/usb_device.c   2>&1  > ${OBJECTDIR}/_ext/1360907413/usb_device.o.err ; if [ $$? -eq 0 ] ; then touch ${OBJECTDIR}/_ext/1360907413/usb_device.o.ok ; fi 
	@touch ${OBJECTDIR}/_ext/1360907413/usb_device.o.d 
	
ifneq (,$(findstring MINGW32,$(OS_CURRENT))) 
	@sed -e 's/\\$$/__EOL__/g' -e 's/\\ /__ESCAPED_SPACES__/g' -e 's/\\/\//g' -e 's/__ESCAPED_SPACES__/\\ /g' -e 's/__EOL__$$/\\/g' ${OBJECTDIR}/_ext/1360907413/usb_device.o.d > ${OBJECTDIR}/_ext/1360907413/usb_device.o.tmp
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_device.o.d 
	@${CP} ${OBJECTDIR}/_ext/1360907413/usb_device.o.tmp ${OBJECTDIR}/_ext/1360907413/usb_device.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_device.o.tmp
endif
	@touch ${OBJECTDIR}/_ext/1360907413/usb_device.o.err 
	@cat ${OBJECTDIR}/_ext/1360907413/usb_device.o.err 
	@if [ -f ${OBJECTDIR}/_ext/1360907413/usb_device.o.ok ] ; then rm -f ${OBJECTDIR}/_ext/1360907413/usb_device.o.ok; else exit 1; fi
	
else
${OBJECTDIR}/_ext/1472/main.o: ../main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.ok ${OBJECTDIR}/_ext/1472/main.o.err 
	@echo ${MP_CC} $(MP_EXTRA_CC_PRE)  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1472/main.o.d -o ${OBJECTDIR}/_ext/1472/main.o ../main.c  
	@-${MP_CC} $(MP_EXTRA_CC_PRE)  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1472/main.o.d -o ${OBJECTDIR}/_ext/1472/main.o ../main.c   2>&1  > ${OBJECTDIR}/_ext/1472/main.o.err ; if [ $$? -eq 0 ] ; then touch ${OBJECTDIR}/_ext/1472/main.o.ok ; fi 
	@touch ${OBJECTDIR}/_ext/1472/main.o.d 
	
ifneq (,$(findstring MINGW32,$(OS_CURRENT))) 
	@sed -e 's/\\$$/__EOL__/g' -e 's/\\ /__ESCAPED_SPACES__/g' -e 's/\\/\//g' -e 's/__ESCAPED_SPACES__/\\ /g' -e 's/__EOL__$$/\\/g' ${OBJECTDIR}/_ext/1472/main.o.d > ${OBJECTDIR}/_ext/1472/main.o.tmp
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.d 
	@${CP} ${OBJECTDIR}/_ext/1472/main.o.tmp ${OBJECTDIR}/_ext/1472/main.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/main.o.tmp
endif
	@touch ${OBJECTDIR}/_ext/1472/main.o.err 
	@cat ${OBJECTDIR}/_ext/1472/main.o.err 
	@if [ -f ${OBJECTDIR}/_ext/1472/main.o.ok ] ; then rm -f ${OBJECTDIR}/_ext/1472/main.o.ok; else exit 1; fi
	
${OBJECTDIR}/_ext/1472/usb_descriptors.o: ../usb_descriptors.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1472 
	@${RM} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.ok ${OBJECTDIR}/_ext/1472/usb_descriptors.o.err 
	@echo ${MP_CC} $(MP_EXTRA_CC_PRE)  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d -o ${OBJECTDIR}/_ext/1472/usb_descriptors.o ../usb_descriptors.c  
	@-${MP_CC} $(MP_EXTRA_CC_PRE)  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d -o ${OBJECTDIR}/_ext/1472/usb_descriptors.o ../usb_descriptors.c   2>&1  > ${OBJECTDIR}/_ext/1472/usb_descriptors.o.err ; if [ $$? -eq 0 ] ; then touch ${OBJECTDIR}/_ext/1472/usb_descriptors.o.ok ; fi 
	@touch ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d 
	
ifneq (,$(findstring MINGW32,$(OS_CURRENT))) 
	@sed -e 's/\\$$/__EOL__/g' -e 's/\\ /__ESCAPED_SPACES__/g' -e 's/\\/\//g' -e 's/__ESCAPED_SPACES__/\\ /g' -e 's/__EOL__$$/\\/g' ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d > ${OBJECTDIR}/_ext/1472/usb_descriptors.o.tmp
	@${RM} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d 
	@${CP} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.tmp ${OBJECTDIR}/_ext/1472/usb_descriptors.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/usb_descriptors.o.tmp
endif
	@touch ${OBJECTDIR}/_ext/1472/usb_descriptors.o.err 
	@cat ${OBJECTDIR}/_ext/1472/usb_descriptors.o.err 
	@if [ -f ${OBJECTDIR}/_ext/1472/usb_descriptors.o.ok ] ; then rm -f ${OBJECTDIR}/_ext/1472/usb_descriptors.o.ok; else exit 1; fi
	
${OBJECTDIR}/_ext/1360907413/usb_function_hid.o: ../USB/usb_function_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1360907413 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.ok ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.err 
	@echo ${MP_CC} $(MP_EXTRA_CC_PRE)  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d -o ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o ../USB/usb_function_hid.c  
	@-${MP_CC} $(MP_EXTRA_CC_PRE)  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d -o ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o ../USB/usb_function_hid.c   2>&1  > ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.err ; if [ $$? -eq 0 ] ; then touch ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.ok ; fi 
	@touch ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d 
	
ifneq (,$(findstring MINGW32,$(OS_CURRENT))) 
	@sed -e 's/\\$$/__EOL__/g' -e 's/\\ /__ESCAPED_SPACES__/g' -e 's/\\/\//g' -e 's/__ESCAPED_SPACES__/\\ /g' -e 's/__EOL__$$/\\/g' ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d > ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.tmp
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d 
	@${CP} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.tmp ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.tmp
endif
	@touch ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.err 
	@cat ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.err 
	@if [ -f ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.ok ] ; then rm -f ${OBJECTDIR}/_ext/1360907413/usb_function_hid.o.ok; else exit 1; fi
	
${OBJECTDIR}/_ext/1360907413/usb_device.o: ../USB/usb_device.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1360907413 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_device.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_device.o.ok ${OBJECTDIR}/_ext/1360907413/usb_device.o.err 
	@echo ${MP_CC} $(MP_EXTRA_CC_PRE)  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1360907413/usb_device.o.d -o ${OBJECTDIR}/_ext/1360907413/usb_device.o ../USB/usb_device.c  
	@-${MP_CC} $(MP_EXTRA_CC_PRE)  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -DMAXIMITE -I".." -I"/C/Embedded/Microchip/Solutions v2011-07-14/Microchip/Include" -I"/C/Archives/Electronics/UBW32/HIDBoot/USB" -I"../USB" -MMD -MF ${OBJECTDIR}/_ext/1360907413/usb_device.o.d -o ${OBJECTDIR}/_ext/1360907413/usb_device.o ../USB/usb_device.c   2>&1  > ${OBJECTDIR}/_ext/1360907413/usb_device.o.err ; if [ $$? -eq 0 ] ; then touch ${OBJECTDIR}/_ext/1360907413/usb_device.o.ok ; fi 
	@touch ${OBJECTDIR}/_ext/1360907413/usb_device.o.d 
	
ifneq (,$(findstring MINGW32,$(OS_CURRENT))) 
	@sed -e 's/\\$$/__EOL__/g' -e 's/\\ /__ESCAPED_SPACES__/g' -e 's/\\/\//g' -e 's/__ESCAPED_SPACES__/\\ /g' -e 's/__EOL__$$/\\/g' ${OBJECTDIR}/_ext/1360907413/usb_device.o.d > ${OBJECTDIR}/_ext/1360907413/usb_device.o.tmp
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_device.o.d 
	@${CP} ${OBJECTDIR}/_ext/1360907413/usb_device.o.tmp ${OBJECTDIR}/_ext/1360907413/usb_device.o.d 
	@${RM} ${OBJECTDIR}/_ext/1360907413/usb_device.o.tmp
endif
	@touch ${OBJECTDIR}/_ext/1360907413/usb_device.o.err 
	@cat ${OBJECTDIR}/_ext/1360907413/usb_device.o.err 
	@if [ -f ${OBJECTDIR}/_ext/1360907413/usb_device.o.ok ] ; then rm -f ${OBJECTDIR}/_ext/1360907413/usb_device.o.ok; else exit 1; fi
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/HIDBoot.X.${IMAGE_TYPE}.elf: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mdebugger -D__MPLAB_DEBUGGER_ICD3=1 -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/HIDBoot.X.${IMAGE_TYPE}.elf ${OBJECTFILES}        -Wl,--defsym=__MPLAB_BUILD=1,--report-mem$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__ICD2RAM=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_ICD3=1,-L"..",-Map="${DISTDIR}/HIDBoot.X.${IMAGE_TYPE}.map" 
else
dist/${CND_CONF}/${IMAGE_TYPE}/HIDBoot.X.${IMAGE_TYPE}.elf: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/HIDBoot.X.${IMAGE_TYPE}.elf ${OBJECTFILES}        -Wl,--defsym=__MPLAB_BUILD=1,--report-mem$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),-L"..",-Map="${DISTDIR}/HIDBoot.X.${IMAGE_TYPE}.map"
	${MP_CC_DIR}\\pic32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/HIDBoot.X.${IMAGE_TYPE}.elf  
endif


# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
