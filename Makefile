-include Config.mk

################ Source files ##########################################

exe	:= $O${name}
srcs	:= $(wildcard *.c)
objs	:= $(addprefix $O,$(srcs:.c=.o))
deps	:= ${objs:.o=.d}
confs	:= Config.mk config.h
oname   := $(notdir $(abspath $O))

################ Compilation ###########################################

.SUFFIXES:
.PHONY: all run clean distclean maintainer-clean

all:	${exe}

run:	${exe}
	@$<

${exe}:	$(filter-out $Omakedefs.o,${objs})
	@echo "Linking $@ ..."
	@${CC} ${ldflags} -o $@ $^ ${libs}

$O%.o:	%.c
	@echo "    Compiling $< ..."
	@${CC} ${cflags} -MMD -MT "$(<:.c=.s) $@" -o $@ -c $<

%.s:	%.c
	@echo "    Compiling $< to assembly ..."
	@${CC} ${cflags} -S -o $@ -c $<

$Omakedefs:	$Omakedefs.o
	@${CC} ${ldflags} -o $@ $^ ${libs}

$Oonames.h:	objects.h | $Omakedefs
	@echo "    Generating $@ ..."
	@$Omakedefs $< > $@

$(filter-out $Omakedefs.o,${objs}):	$Oonames.h

################ Installation ##########################################

.PHONY:	install installdirs
.PHONY: uninstall uninstall-man uninstall-pam uninstall-svc

ifdef bindir
exed	:= ${DESTDIR}${bindir}
exei	:= ${exed}/$(notdir ${exe})

${exed}:
	@echo "Creating $@ ..."
	@${INSTALL} -d $@
${exei}:	${exe} | ${exed}
	@echo "Installing $@ ..."
	@${INSTALL_PROGRAM} $< $@

installdirs:	${exed}
install:	${exei}
uninstall:
	@if [ -f ${exei} ]; then\
	    echo "Removing ${exei} ...";\
	    rm -f ${exei};\
	fi
endif
ifdef man6dir
mand	:= ${DESTDIR}${man6dir}
mani	:= ${mand}/${name}.6

${mand}:
	@echo "Creating $@ ..."
	@${INSTALL} -d $@
${mani}:	${name}.6 | ${mand}
	@echo "Installing $@ ..."
	@${INSTALL_DATA} $< $@

installdirs:	${mand}
install:	${mani}
uninstall:	uninstall-man
uninstall-man:
	@if [ -f ${mani} ]; then\
	    echo "Removing ${mani} ...";\
	    rm -f ${mani};\
	fi
endif
ifdef scoredir
scored	:= ${DESTDIR}${scoredir}
scorei	:= ${scored}/${name}.scores

${scored}:
	@echo "Creating $@ ..."
	@${INSTALL} -d $@
${scorei}:	| ${scored}
	@echo "Installing $@ ..."
	@${INSTALL_SCORES} $@

installdirs:	${scored}
install:	${scorei}
endif

################ Maintenance ###########################################

clean:
	@if [ -d ${builddir} ]; then\
	    rm -f ${exe} ${objs} ${deps} $Omakedefs $Oonames.h $O.d;\
	    rmdir ${builddir};\
	fi

distclean:	clean
	@rm -f ${oname} ${confs} config.status

maintainer-clean: distclean

$O.d:	${builddir}/.d
	@[ -h ${oname} ] || ln -sf ${builddir} ${oname}
$O%/.d:	$O.d
	@[ -d $(dir $@) ] || mkdir $(dir $@)
	@touch $@
${builddir}/.d:	Makefile
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	@touch $@

Config.mk:	Config.mk.in
config.h:	config.h.in
${objs}:	Makefile ${confs} $O.d
${confs}:	configure
	@if [ -x config.status ]; then echo "Reconfiguring ...";\
	    ./config.status;\
	else echo "Running configure ...";\
	    ./configure;\
	fi

-include ${dep}
