# TW - Mailer
#### Implemented by Viktor Szüsz and Jakob Friedl
---

## Starting the program

*install and setup LDAP*
```bash
    sudo apt install libldap2-dev ldap-utils
    sudo cp ./config/ldap.conf /etc/ldap/ldap.conf
```

*compile files:*
```bash 
    make
```
*start server (use sudo if using ports below 1024):*
```bash
    ./twmailer-server <port> <mail-spool-directory>
```
*start client:*
```bash
    ./twmailer-client <ip> <port>
```
