# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Werr -g

# Portul pe care asculta serverul (de completat)
PORT = 8082

# Adresa IP a serverului (de completat)
IP_SERVER = 192.168.1.149

ID_CLIENT = boss

all: server subscriber

# Compileaza server.c
server: server.cpp helpers.h

# Compileaza client.c
subscriber: subscriber.cpp

.PHONY: clean run_server run_subscriber


# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run: subscriber
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT}

boss: subscriber
	gnome-terminal --title="Subscriber boss" --geometry=101x15-0+250 -e "make run_subscriber" >/dev/null
ana:
	gnome-terminal --title="Subscriber ana" --geometry=101x15-0+430 -e "make ID_CLIENT=ana run_subscriber" >/dev/null


clean:
	rm -f server subscriber
