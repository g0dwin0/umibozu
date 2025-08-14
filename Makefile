CPPFLAGS = -g -Ilib/imgui -Ilib/ -Ilib/imgui/backends -Ilib/tinyfiledialogs  -Iinclude/core -Iinclude/frontend -Iinclude/ -std=c++20 -Wall -Wextra  -Werror -lpthread -lSDL2 -lGL -Wno-deprecated-enum-enum-conversion

OBJ_DIR = build/obj
OBJ_IMGUI = $(OBJ_DIR)/imgui
OBJS_IMGUI = $(OBJ_IMGUI)/imgui_demo.o $(OBJ_IMGUI)/imgui_draw.o $(OBJ_IMGUI)/imgui_impl_sdlrenderer2.o $(OBJ_IMGUI)/imgui_impl_sdl2.o $(OBJ_IMGUI)/imgui_tables.o $(OBJ_IMGUI)/imgui_widgets.o $(OBJ_IMGUI)/imgui.o
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/cart.o $(OBJ_DIR)/cpu.o $(OBJ_DIR)/ppu.o  $(OBJ_DIR)/bus.o $(OBJ_DIR)/gb.o $(OBJ_DIR)/frontend.o $(OBJ_DIR)/file_dialog.o $(OBJ_DIR)/mappers.o  $(OBJ_DIR)/instructions.o $(OBJ_DIR)/apu.o $(OBJ_DIR)/timer.o $(OBJ_DIR)/double_buffer.o
CC=g++
CPPFLAGS += -O2

all: final

final: $(OBJS)
	@echo "linking...."
	$(CC) $(OBJS) $(OBJS_IMGUI) -o build/bin/umibozu $(CPPFLAGS)
	chmod +x build/bin/umibozu

$(OBJ_DIR)/main.o: src/main.cpp include/common.hpp
	$(CC) $(CPPFLAGS) -c src/main.cpp -Ilib/cli11 -o $(OBJ_DIR)/main.o

$(OBJ_DIR)/cart.o: src/core/cart.cpp include/core/cart.hpp
	$(CC) $(CPPFLAGS) -c src/core/cart.cpp -o $(OBJ_DIR)/cart.o

$(OBJ_DIR)/cpu.o: src/core/cpu.cpp include/core/cpu.hpp
	$(CC) $(CPPFLAGS) -c src/core/cpu.cpp -o $(OBJ_DIR)/cpu.o

$(OBJ_DIR)/bus.o: src/core/bus.cpp include/core/bus.hpp
	$(CC) $(CPPFLAGS) -c src/core/bus.cpp -o $(OBJ_DIR)/bus.o

$(OBJ_DIR)/gb.o: src/core/gb.cpp include/core/gb.hpp
	$(CC) $(CPPFLAGS) -c src/core/gb.cpp -o $(OBJ_DIR)/gb.o

$(OBJ_DIR)/ppu.o: src/core/ppu.cpp include/core/ppu.hpp
	$(CC) $(CPPFLAGS) -c src/core/ppu.cpp -o $(OBJ_DIR)/ppu.o

$(OBJ_DIR)/file_dialog.o: lib/tinyfiledialogs/tinyfiledialogs.c lib/tinyfiledialogs/tinyfiledialogs.h
	$(CC) $(CPPFLAGS) -c lib/tinyfiledialogs/tinyfiledialogs.c -o $(OBJ_DIR)/file_dialog.o -Wno-unused-variable

$(OBJ_DIR)/mappers.o: src/core/mappers.cpp src/core/mappers/mbc1.cpp src/core/mappers/mbc3.cpp src/core/mappers/mbc5.cpp include/core/mapper.hpp
	$(CC) $(CPPFLAGS) -c src/core/mappers.cpp -o $(OBJ_DIR)/mappers.o

$(OBJ_DIR)/frontend.o: src/frontend/window.cpp include/frontend/window.hpp 
	$(CC) $(CPPFLAGS) -c src/frontend/window.cpp -o $(OBJ_DIR)/frontend.o $(VERSION_FLAGS)

$(OBJ_DIR)/instructions.o: src/core/instructions.cpp include/core/instructions.hpp
	$(CC) $(CPPFLAGS) -c src/core/instructions.cpp -o $(OBJ_DIR)/instructions.o

$(OBJ_DIR)/apu.o: src/core/apu.cpp include/core/apu.hpp
	$(CC) $(CPPFLAGS) -c src/core/apu.cpp -o $(OBJ_DIR)/apu.o

$(OBJ_DIR)/timer.o: src/core/timer.cpp include/core/timer.hpp
	$(CC) $(CPPFLAGS) -c src/core/timer.cpp -o $(OBJ_DIR)/timer.o

$(OBJ_DIR)/double_buffer.o: src/core/double_buffer.cpp include/core/double_buffer.hpp
	$(CC) $(CPPFLAGS) -c src/core/double_buffer.cpp -o $(OBJ_DIR)/double_buffer.o


check: tests/cpu_tests.cpp
	$(CC) tests/cpu_tests.cpp src/core/cpu.cpp src/core/bus.cpp src/core/instructions.cpp  src/core/timer.cpp src/core/apu.cpp  -O2 -DCPU_TEST_MODE_H -Ilib/ -Iinclude -Iinclude/core -std=c++20 -o build/bin/tests
	build/bin/tests

clean:
	rm $(OBJS)

umibozu:
	cat /dev/null > MyLog.txt
	./build/bin/umibozu >> MyLog.txt