TARGET_TRACKER=p2p-tracker
TARGET_PEER=p2p-peer
CC=gcc
WARN=-Wall
PTHREAD=-pthread
LUUID=-luuid
FLAGS=$(WARN) $(PTHREAD)  $(LUUID)

all: $(TARGET_TRACKER) $(TARGET_PEER)

$(TARGET_TRACKER): tracker.o session.o
	$(CC) tracker.o session.o -o $(TARGET_TRACKER) $(FLAGS)

$(TARGET_PEER): peer.o
	$(CC) peer.o -o $(TARGET_PEER) $(FLAGS)

tracker.o: tracker.c env.h session.h
	$(CC) -c tracker.c -o tracker.o $(FLAGS)

session.o: session.c session.h
	$(CC) -c session.c -o session.o $(FLAGS)

peer.o: peer.c env.h
	$(CC) -c peer.c -o peer.o $(FLAGS)

clean:
	rm -rf *.o $(TARGET_TRACKER) $(TARGET_PEER)