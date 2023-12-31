CPPFLAGS = -g -Ilib/imgui -Ilib/ -Ilib/imgui/backends -Ilib/tinyfiledialogs  -Iinclude/core -Iinclude/frontend -Iinclude/ -std=c++20 -Wall -Wextra -Wpedantic -Werror -lpthread -lSDL2 -lGL -Wno-deprecated-enum-enum-conversion

OBJ_DIR = build/obj
OBJ_IMGUI = $(OBJ_DIR)/imgui
OBJS_IMGUI = $(OBJ_IMGUI)/imgui_demo.o $(OBJ_IMGUI)/imgui_draw.o $(OBJ_IMGUI)/imgui_impl_sdlrenderer2.o $(OBJ_IMGUI)/imgui_impl_sdl2.o $(OBJ_IMGUI)/imgui_tables.o $(OBJ_IMGUI)/imgui_widgets.o $(OBJ_IMGUI)/imgui.o
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/cart.o $(OBJ_DIR)/cpu.o $(OBJ_DIR)/ppu.o  $(OBJ_DIR)/bus.o $(OBJ_DIR)/gb.o $(OBJ_DIR)/frontend.o $(OBJ_DIR)/file_dialog.o $(OBJ_DIR)/mappers.o 
CC=g++
CPPFLAGS += -O2
# CPPFLAGS += -fsanitize=undefined,address -D_GLIBCXX_DEBUG


all: final

final: $(OBJS)
	@echo "linking...."
	$(CC) $(OBJS) $(OBJS_IMGUI) -o build/bin/umibozu $(CPPFLAGS)
	chmod +x build/bin/umibozu

$(OBJ_DIR)/main.o: src/main.cpp include/common.h
	$(CC) $(CPPFLAGS) -c src/main.cpp -Ilib/cli11 -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/cart.o: src/core/cart.cpp include/core/cart.h
	$(CC) $(CPPFLAGS) -c src/core/cart.cpp -o $(OBJ_DIR)/cart.o

$(OBJ_DIR)/cpu.o: src/core/cpu.cpp include/core/cpu.h
	$(CC) $(CPPFLAGS) -c src/core/cpu.cpp -o $(OBJ_DIR)/cpu.o

$(OBJ_DIR)/bus.o: src/core/bus.cpp include/core/bus.h
	$(CC) $(CPPFLAGS) -c src/core/bus.cpp -o $(OBJ_DIR)/bus.o

$(OBJ_DIR)/gb.o: src/core/gb.cpp include/core/gb.h
	$(CC) $(CPPFLAGS) -c src/core/gb.cpp -o $(OBJ_DIR)/gb.o

$(OBJ_DIR)/ppu.o: src/core/ppu.cpp include/core/ppu.h
	$(CC) $(CPPFLAGS) -c src/core/ppu.cpp -o $(OBJ_DIR)/ppu.o

$(OBJ_DIR)/file_dialog.o: lib/tinyfiledialogs/tinyfiledialogs.c lib/tinyfiledialogs/tinyfiledialogs.h
	$(CC) $(CPPFLAGS) -c lib/tinyfiledialogs/tinyfiledialogs.c -o $(OBJ_DIR)/file_dialog.o -Wno-unused-variable

$(OBJ_DIR)/mappers.o: src/core/mappers.cpp include/core/mapper.h
	$(CC) $(CPPFLAGS) -c src/core/mappers.cpp -o $(OBJ_DIR)/mappers.o

$(OBJ_DIR)/frontend.o: src/frontend/window.cpp include/frontend/window.h
	$(CC) $(CPPFLAGS) -c src/frontend/window.cpp -o $(OBJ_DIR)/frontend.o


check: tests/cpu_tests.cpp
	$(CC) tests/cpu_tests.cpp src/core/cpu.cpp src/core/bus.cpp -O2 -DCPU_TEST_MODE_H -Ilib/ -Iinclude -Iinclude/core -std=c++20 -o build/bin/tests
	time build/bin/tests

clean:
	rm $(OBJS)

umibozu:
	cat /dev/null > MyLog.txt
	./build/bin/umibozu >> MyLog.txt