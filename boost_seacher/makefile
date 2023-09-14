PARSER=parser
DUG=debug
HTTP_SERVER =http_server
cc=g++

.PHONY:all
all: $(PARSER) $(DUG) $(HTTP_SERVER)

$(HTTP_SERVER):http_server.cc
	$(cc) -o $@ $^ -lboost_system -lpthread -ljsoncpp  -std=c++11
$(PARSER):Parser.cc
	$(cc) -o $@ $^ -lboost_system -lboost_filesystem -std=c++11
$(DUG):debug.cc
	$(cc) -o $@ $^ -ljsoncpp -lpthread  -std=c++11
.PHONY:clean
clean:
	rm -f $(PARSER) $(DUG) $(HTTP_SERVER) 
