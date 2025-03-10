.PHONY: build start run

build:
	@ecpg -o ecpg.c main.c
	@gcc ecpg.c -o app -I/usr/include/postgresql -lecpg

start:
	@./app

run:
	@make build
	@make start