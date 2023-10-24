umibozu:
	cat /dev/null > MyLog.txt
	g++ -Iinclude src/*.cpp src/core/**.cpp -o build/umibozu -std=c++20
	./build/umibozu >> MyLog.txt