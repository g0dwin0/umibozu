CPPFLAGS = -Iinclude/ -Iinclude/core -Iinclude/frontend -std=c++20 -Wall -Wextra -Wpedantic -Werror -lpthread -lSDL2
OBJ_DIR = build/obj
FINAL_FILES = $(OBJ_DIR)/main.o $(OBJ_DIR)/cart.o $(OBJ_DIR)/cpu.o $(OBJ_DIR)/bus.o $(OBJ_DIR)/gb.o $(OBJ_DIR)/frontend.o 
CC=g++

all: final

final: $(FINAL_FILES)
	@echo "linking...."
	$(CC) $(FINAL_FILES) -o build/bin/umibozu $(CPPFLAGS)
	chmod +x build/bin/umibozu

$(OBJ_DIR)/main.o: src/main.cpp
	@echo "compiling main"
	$(CC) $(CPPFLAGS) -c src/main.cpp -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/cart.o: src/core/cart.cpp include/core/cart/cart.h
	$(CC) $(CPPFLAGS) -c src/core/cart.cpp -o $(OBJ_DIR)/cart.o

$(OBJ_DIR)/cpu.o: src/core/cpu.cpp include/core/cpu/cpu.h
	$(CC) $(CPPFLAGS) -c src/core/cpu.cpp -o $(OBJ_DIR)/cpu.o

$(OBJ_DIR)/bus.o: src/core/bus.cpp include/core/bus.h
	$(CC) $(CPPFLAGS) -c src/core/bus.cpp -o $(OBJ_DIR)/bus.o

$(OBJ_DIR)/gb.o: src/core/gb.cpp include/core/gb.h
	$(CC) $(CPPFLAGS) -c src/core/gb.cpp -o $(OBJ_DIR)/gb.o

$(OBJ_DIR)/frontend.o: src/frontend/window.cpp include/frontend/window.h
	$(CC) $(CPPFLAGS) -c src/frontend/window.cpp -o $(OBJ_DIR)/frontend.o




clean:
	rm $(OBJ_DIR)/cart.o $(OBJ_DIR)/main.o $(OBJ_DIR)/cpu.o $(OBJ_DIR)/bus.o $(OBJ_DIR)/gb.o $(OBJ_DIR)/frontend.o

umibozu:
	cat /dev/null > MyLog.txt
	./build/bin/umibozu >> MyLog.txt