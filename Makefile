all:
	@echo "=== Building libopticon ============================================="
	@cd src/libopticon && make && cd ../..
	@echo "=== Building libopticondb ==========================================="
	@cd src/libopticondb && make && cd ../..
	@echo "=== Building libsvc ================================================="
	@cd src/libsvc && make && cd ../..
	@echo "=== Building libopticonf ============================================"
	@cd src/libopticonf && make && cd ../..

test: all
	@cd test && make test && cd ..

clean:
	@cd src/libopticon && make clean && cd ../..
	@cd src/libopticondb && make clean && cd ../..
	@cd src/libsvc && make clean && cd ../..
	@cd test && make clean && cd ..
