CPPFLAGS = -Iinclude -std=c++20
OBJ_DIR = build/obj
FINAL_FILES = $(OBJ_DIR)/main.o $(OBJ_DIR)/cart.o $(OBJ_DIR)/cpu.o
CC=g++

all: final

final: $(FINAL_FILES)
	@echo "linking...."
	$(CC) $(CLFAGS) $(FINAL_FILES) -o build/bin/umibozu
	chmod +x build/bin/umibozu

$(OBJ_DIR)/main.o: src/main.cpp
	@echo "compiling main"
	$(CC) $(CPPFLAGS) -c src/main.cpp -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/cart.o: src/core/cart.cpp
	@echo "compiling cart..."
	$(CC) $(CPPFLAGS) -c src/core/cart.cpp -o $(OBJ_DIR)/cart.o

$(OBJ_DIR)/cpu.o: src/core/cpu.cpp
	@echo "compiling cpu..."
	$(CC) $(CPPFLAGS) -c src/core/cpu.cpp -o $(OBJ_DIR)/cpu.o

clean:
	rm $(OBJ_DIR)/cart.o $(OBJ_DIR)/main.o $(OBJ_DIR)/cpu.o

umibozu:
	cat /dev/null > MyLog.txt
## $(CC) -Iinclude src/*.cpp src/core/**.cpp -o build/umibozu -std=c++20
	./build/bin/umibozu >> MyLog.txt