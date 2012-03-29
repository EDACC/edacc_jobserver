CFLAGS=-ggdb -g -W -Wall -Wextra `mysql_config --cflags` -O2
LDFLAGS=`mysql_config --libs` -lpthread -lz


CC = g++
COMPILE= $(CC) $(CFLAGS) -c

OBJ_FILES=edacc_jobserver.o database.o log.o joblist.o experimentlist.o databaselist.o md5sum.o statistics.o

all: edacc_jobserver

edacc_jobserver: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o edacc_jobserver $(LDFLAGS)
	
edacc_jobserver.o: edacc_jobserver.cpp *.hpp
	$(COMPILE) edacc_jobserver.cpp
	
database.o: database.cpp database.hpp
	$(COMPILE) database.cpp
	
log.o: log.cpp log.hpp
	$(COMPILE) log.cpp
	
joblist.o: joblist.cpp joblist.hpp
	$(COMPILE) joblist.cpp
	
experimentlist.o: experimentlist.cpp experimentlist.hpp
	$(COMPILE) experimentlist.cpp

databaselist.o: databaselist.cpp databaselist.hpp
	$(COMPILE) databaselist.cpp
	
md5sum.o: md5sum.c md5sum.h
	$(COMPILE) md5sum.c
	
statistics.o: statistics.cpp statistics.hpp
	$(COMPILE) statistics.cpp
clean:
	@rm -rf *.o
