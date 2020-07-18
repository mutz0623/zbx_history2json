CC = gcc
CFLAG = -MMD -MP -Wall -Wextra -O3 -fPIC
LDFLAG = -shared
INCLUDE = -I../zabbix-src/include
SRCDIR = ./src
SRC = $(wildcard $(SRCDIR)/*.c)
OBJDIR = ./obj
OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRC:.c=.o)))
BINDIR = ./bin
BIN = history2json.so
TARGET = $(BINDIR)/$(BIN)
DEPENDS = $(OBJ:.o=.d)
MODULEPATH = /etc/zabbix/modules
MODULECONF = history2json.conf

$(TARGET): $(OBJ)
	-mkdir -p $(BINDIR)
	$(CC) $(LDFLAG) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	-mkdir -p $(OBJDIR)
	$(CC) $(CFLAG) $(INCLUDE) -o $@ -c $<

-include $(DEPENDS)

.PHONY: all clean
all: clean $(TARGET)

clean:
	-rm -f $(TARGET) $(OBJ) $(DEPENDS)

.PHONY: install test
install:$(TARGET)
	test $$(whoami) == "root"
	service zabbix-server stop
	[ -d $(MODULEPATH) ] || mkdir $(MODULEPATH)
	install -C $(TARGET) $(MODULEPATH)
	ls $(MODULEPATH)/$(MODULECONF) || cp $(MODULECONF) $(MODULEPATH)
	service zabbix-server start
	service zabbix-server status
	sleep 0.5
	tail /var/log/zabbix/zabbix_server.log

test:$(target)
	md5sum  $(MODULEPATH)/$(TARGET) ./$(TARGET) || :

.PHONY: log-level-up log-level-down check source-config
log-level-up:
	zabbix_server --runtime-control log_level_increase="history syncer"

log-level-down:
	zabbix_server --runtime-control log_level_decrease="history syncer"

check:
	test $$(whoami) == "root"
	cat /proc/$$(cat /run/zabbix/zabbix_server.pid)/environ | strings |grep LD_PRELOAD  | wc -l
	cat /etc/sysconfig/zabbix-server |grep -v -e "^$$" -e "^#"  || :
	cat /etc/zabbix/modules/history2json.conf |grep -v -e "^$$" -e "^#"

source-config:
	cd ../zabbix-src/; sh ./configure --build=x86_64-redhat-linux-gnu --host=x86_64-redhat-linux-gnu --program-prefix= --disable-dependency-tracking --prefix=/usr --exec-prefix=/usr --bindir=/usr/bin --sbindir=/usr/sbin --sysconfdir=/etc --datadir=/usr/share --includedir=/usr/include --libdir=/usr/lib64 --libexecdir=/usr/libexec --localstatedir=/var --sharedstatedir=/var/lib --mandir=/usr/share/man --infodir=/usr/share/info --enable-dependency-tracking --sysconfdir=/etc/zabbix --libdir=/usr/lib64/zabbix --enable-agent --enable-proxy --enable-ipv6 --enable-java --with-net-snmp --with-ldap --with-libcurl --with-openipmi --with-unixodbc --with-ssh2 --with-libxml2 --with-libevent --with-libpcre --with-openssl --enable-server --with-jabber --with-postgresql

.PHONY :doc doc-clean
doc:
	doxygen ./doc/Doxyfile

doc-clean:
	-rm -rf ./doc/html/

.PHONY : scan-build scan-build-clean
scan-build:clean
	-mkdir -p ./doc/scan-build/
	scan-build -o ./doc/scan-build/ $(MAKE)

scan-build-clean:
	-rm -rf ./doc/scan-build/*

.PHONY: clean-all
clean-all: clean doc-clean scan-build-clean
	$(MAKE) clean
	$(MAKE) doc-clean
	$(MAKE) scan-build-clean

