all:
	@echo "=== Building libopticon ============================================="
	@cd libopticon && make && cd ..
	@echo "=== Building libopticondb ==========================================="
	@cd libopticondb && make && cd ..
	@echo "=== Building tests =================================================="
	@cd test && make && cd ..

clean:
	@cd libopticon && make clean && cd ..
	@cd libopticondb && make clean && cd ..
	@cd test && make clean && cd ..
