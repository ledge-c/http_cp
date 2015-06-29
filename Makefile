# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc

# Object Files
OBJECTFILES= \
	common_funcs.o \
	htfile.o \
	network.o

CFLAGS=-Wall
all http_cp: CFLAGS += -O2
debug: CFLAGS += -g


all http_cp: ${OBJECTFILES} depcheck
	${LINK.c} -o http_cp ${OBJECTFILES} ${LDLIBSOPTIONS}

debug: ${OBJECTFILES} depcheck
	${LINK.c} -o http_cp ${OBJECTFILES} ${LDLIBSOPTIONS}

common_funcs.o: common_funcs.c 
	${RM} "$@.d"
	$(COMPILE.c) -MMD -MP -MF "$@.d" -o common_funcs.o common_funcs.c

htfile.o: htfile.c 
	${RM} "$@.d"
	$(COMPILE.c) -MMD -MP -MF "$@.d" -o htfile.o htfile.c

network.o: network.c 
	${RM} "$@.d"
	$(COMPILE.c) -MMD -MP -MF "$@.d" -o network.o network.c
	
clean:
	${RM} ${OBJECTFILES} ${OBJECTFILES:=.d}
	${RM} http_cp
	
# Dependency checking
depcheck:
	@if [ -n "${MAKE_VERSION}" ]; then \
	    echo "DEPFILES=\$$(wildcard \$$(addsuffix .d, \$${OBJECTFILES}))" >>.dep.inc; \
	    echo "ifneq (\$${DEPFILES},)" >>.dep.inc; \
	    echo "include \$${DEPFILES}" >>.dep.inc; \
	    echo "endif" >>.dep.inc; \
	else \
	    echo ".KEEP_STATE:" >>.dep.inc; \
	    echo ".KEEP_STATE_FILE:.make.state.\$${CONF}" >>.dep.inc; \
	fi
	
include .dep.inc

