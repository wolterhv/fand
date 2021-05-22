SHELL:=/bin/bash

compile:
	gcc -g -lm -lrt -lpigpiod_if2 -linih -o fand cpu_interface_generic.c fan_interface_pigpiodif2.c fand.c

send:
	scp Makefile                    wolter@rastapi.local:fand/
	scp fand.c                      wolter@rastapi.local:fand/
	scp fand.h                      wolter@rastapi.local:fand/
	scp fan_interface.h             wolter@rastapi.local:fand/
	scp fan_interface_pigpiodif2.c  wolter@rastapi.local:fand/
	scp cpu_interface.h             wolter@rastapi.local:fand/
	scp cpu_interface_generic.c     wolter@rastapi.local:fand/
	scp config.ini                  wolter@rastapi.local:fand/
