all:
	@cd libopticon && make && cd ..
	@cd test && make && cd ..

clean:
	@cd libopticon && make clean && cd ..
	@cd test && make clean && cd ..
