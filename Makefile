TARGET_TRACKER=p2p-tracker
TARGET_PEER=p2p-peer
CC=gcc
DEBUG=-g
OPT=-O0
WARN=-Wall
PTHREAD=-pthread
FLAGS=$(DEBUG) $(PTHREAD) $(OPT) $(WARN)
SOURCE_CODE_TRACKER=tracker.c session.h env.h
SOURCE_CODE_PEER=peer.c env.h

tracker:
	$(CC) $(FLAGS) $(SOURCE_CODE_TRACKER) -o $(TARGET_TRACKER)

peer:
	$(CC) $(FLAGS) $(SOURCE_CODE_PEER) -o $(TARGET_PEER)
