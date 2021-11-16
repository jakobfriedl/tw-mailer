# TW - Mailer
#### Implemented by Viktor Szüsz and Jakob Friedl
---

## Starting the program

*install UUID-library*
```bash
    sudo apt-get uuid-dev
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
