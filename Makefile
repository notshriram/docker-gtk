all: build

src/main:
	g++ --std=c++20 main.cpp -o outline `pkg-config --cflags --libs gtk+-3.0` 

.PHONY: build
build:
	docker build -t shriram/docker-cairo:latest -f deployment/Dockerfile .
	@echo ::: built :::

.PHONY: run
run:
	docker run -ti --rm -e DISPLAY=$(DISPLAY) -v /tmp/.X11-unix:/tmp/.X11-unix shriram/docker-cairo:latest

.PHONY: push
push:
	docker push shriram/docker-cairo:latest

.PHONY: clean
clean:
	@rm -f ./main
	@echo ::: cleaned :::
