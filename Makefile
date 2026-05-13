BUILD_DIR := build

.PHONY: configure build clean run compile-commands

configure:
	cmake -B $(BUILD_DIR) -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	ln -sf $(BUILD_DIR)/compile_commands.json compile_commands.json

build:
	cmake --build $(BUILD_DIR) -j

run: build
	./$(BUILD_DIR)/RedlineSS_artefacts/Release/Standalone/RedlineSS

clean:
	rm -rf $(BUILD_DIR) compile_commands.json

compile-commands: configure
