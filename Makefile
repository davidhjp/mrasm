CFLAGS= -c -g -fPIC
CFLAGS2= -I/cygdrive/c/Program\ Files\ \(x86\)/Java/jdk1.6.0_21/include -I/cygdrive/c/Program\ Files\ \(x86\)/Java/jdk1.6.0_21/include/win32
all : clean iniparser.o machine.o table.o macstack.o y.tab.o lex.yy.o mrasm.o socket_io.o main.o
	gcc *.o -lfl -lstdc++ -o mrasm.exe
	mkdir -p mrasm && mv mrasm.exe mrasm && cp newr.ini mrasm
y.tab.c y.tab.h : yaccer.y
	yacc -d yaccer.y

lex.yy.c : lexer.l
	flex lexer.l

%.o : %.c
	gcc $(CFLAGS) $(CFLAGS2) $<

m2.o : m2.c 
	gcc $(CFLAGS) $(CFLAGS2) m2.c

simulate.o : simulate.cpp simulate.h data.h control.h
	gcc $(CFLAGS) $(CFLAGS2) simulate.cpp 

data.o : data.cpp data.h
	gcc $(CFLAGS) $(CFLAGS2) data.cpp

control.o : control.cpp control.h
	gcc $(CFLAGS) $(CFLAGS2) control.cpp 

libmrasm.so : iniparser.o machine.o table.o macstack.o y.tab.o lex.yy.o mrasm.o socket_io.o main.o
	gcc *.o -g -shared -lfl -lstdc++ -o libmrasm.so -d
clean :
	rm -rfv y.tab.c y.tab.h lex.yy.c libmrasm.so *.o mrasm
