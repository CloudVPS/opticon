all:
	@echo "=== Building libopticon ============================================="
	@cd src/libopticon && make && cd ../..
	@echo "=== Building libopticondb ==========================================="
	@cd src/libopticondb && make && cd ../..
	@echo "=== Building libhttp ================================================"
	@cd src/libhttp && make && cd ../..
	@echo "=== Building opticon-agent =========================================="
	@cd src/opticon-agent && make && cd ../..
	@echo "=== Building opticon-api ============================================"
	@cd src/opticon-api && make && cd ../..
	@echo "=== Building opticon-db ============================================="
	@cd src/opticon-db && make && cd ../..
	@echo "=== Building opticon-collector ======================================"
	@cd src/opticon-collector && make && cd ../..

test: all
	@cd test && make test && cd ..

clean:
	@cd src/libopticon && make clean && cd ../..
	@cd src/libopticondb && make clean && cd ../..
	@cd src/libhttp && make clean && cd ../..
	@cd src/opticon-db && make clean && cd ../..
	@cd src/opticon-api && make clean && cd ../..
	@cd src/opticon-agent && make clean && cd ../..
	@cd src/opticon-collector && make clean && cd ../..
	@cd test && make clean && cd ..
