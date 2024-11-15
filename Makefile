# Compiler and flags
CC = gcc
CFLAGS =

# Target executable, source file, and config file
TARGET = /bin/ai
SRC = ai.c
CONFIG_DIR = /etc/ai
CONFIG_FILE = $(CONFIG_DIR)/ai.conf
DEFAULT_CONFIG = ai-default.conf

# Default target
all: check_pkgconfig check_jsonc check_curl $(TARGET) install_config

# Check if pkg-config is installed
check_pkgconfig:
        @which pkg-config > /dev/null || (echo "pkg-config not found. Please install it with 'sudo apt install pkg-config'"; exit 1)

# Check if libjson-c is installed
check_jsonc:
        @pkg-config --exists json-c || (echo "libjson-c not found. Please install it with 'sudo apt install libjson-c-dev'"; exit 1)

# Check if libcurl is installed
check_curl:
        @pkg-config --exists libcurl || (echo "libcurl not found. Please install it with 'sudo apt install libcurl4-openssl-dev'"; exit 1)

# Build target
$(TARGET): $(SRC)
        $(CC) $(SRC) $(CFLAGS) -o ai -ljson-c -lcurl
        sudo mv ai $(TARGET)
        sudo chmod +x $(TARGET)

# Install default config
install_config:
        @if [ ! -d $(CONFIG_DIR) ]; then \
                sudo mkdir -p $(CONFIG_DIR); \
                echo "Created directory $(CONFIG_DIR)"; \
        fi
        @if [ -f $(DEFAULT_CONFIG) ]; then \
                sudo cp $(DEFAULT_CONFIG) $(CONFIG_FILE); \
                echo "Installed config file to $(CONFIG_FILE)"; \
        else \
                echo "Warning: Default config file $(DEFAULT_CONFIG) not found."; \
        fi

# Clean up the local build file
clean:
        rm -f ai

# Uninstall target
uninstall:
        @echo "Removing $(TARGET)..."
        @sudo rm -f $(TARGET)
        @echo "Removing $(CONFIG_DIR) and all contents..."
        @sudo rm -rf $(CONFIG_DIR)
        @echo "Uninstallation complete."

