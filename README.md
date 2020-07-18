# zbx_history2json

- real time exporting module for zabbix history data, using zabbix loadable module.
- export JSON formatted text file to configured path.
- could split files by date and item types in settings.

# Requirements
- zabbix 3.4 or later

# How to use

Below is example for zabbix 3.4.8.
Please replace correct version for your environment.

downlaod and extract zabbix source code
```
$ wget http://downloads.sourceforge.net/project/zabbix/ZABBIX%20Latest%20Stable/3.4.8/zabbix-3.4.8.tar.gz
$ tar zxf zabbix-3.4.8.tar.gz
```

create symlink for zabbix source dir
```
$ ln -s zabbix-3.4.8 zabbix-src
```

download or git clone the code of this module, and make this.
```
$ git clone https://github.com/mutz0623/zbx_history2json.git
$ cd zbx_history2json
$ make source-config
$ make
```

modify zabbix_server.conf
 (LoadModulePath,LoadModule)
```
$ vi /etc/zabbix/zabbix_server.conf
```

install module and restart daemon.
```
$ sudo make install 
```


# misc
- When `./configure` in zabbix source directory, specify same options when compling zabbix server binaries.
    - In ZABBIX SIA official build RPM for RHEL/CentOS7, it was below.
`./configure --build=x86_64-redhat-linux-gnu --host=x86_64-redhat-linux-gnu --program-prefix= --disable-dependency-tracking --prefix=/usr --exec-prefix=/usr --bindir=/usr/bin --sbindir=/usr/sbin --sysconfdir=/etc --datadir=/usr/share --includedir=/usr/include --libdir=/usr/lib64 --libexecdir=/usr/libexec --localstatedir=/var --sharedstatedir=/var/lib --mandir=/usr/share/man --infodir=/usr/share/info --enable-dependency-tracking --sysconfdir=/etc/zabbix --libdir=/usr/lib64/zabbix --enable-agent --enable-proxy --enable-ipv6 --enable-java --with-net-snmp --with-ldap --with-libcurl --with-openipmi --with-unixodbc --with-ssh2 --with-libxml2 --with-libevent --with-libpcre --with-openssl --enable-server --with-jabber --with-postgresql`

