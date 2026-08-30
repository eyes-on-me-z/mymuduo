all: testserver stress_client

testserver : testserver.cc
	g++ -o $@ $^ -std=c++17 -lmymuduo -lpthread

stress_client : stress_client.cc
	g++ -o $@ $^ -std=c++17 -O2 -lpthread

.PHONY:clean
clean:
	rm -f testserver stress_client
