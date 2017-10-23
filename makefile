all: check 
default: check
check: tmaster
clean: 
	rm -rf worker worker.o master master.o
tworker: clean worker
	./worker -x $(X) -n $(N)
tmaster: clean master worker
	./master --worker_path ./worker --wait_mechanism ${MECHANISM} -x $(X) -n $(N)
gdbm:
	gdb --args ./master --worker_path --wait_mechanism ${MECHANISM} ./worker -x $(X) -n $(N)
gdbw: 
	gdb --args ./worker -x $(X) -n $(N)
worker.o: worker.c
	gcc -c -Wall -Werror -fpic -o worker.o worker.c
worker: worker.o
	gcc -g -o worker worker.o
master.o: master.c
	gcc -c -Wall -Werror -fpic -o master.o master.c
master: master.o
	gcc -g -o master master.o
dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
