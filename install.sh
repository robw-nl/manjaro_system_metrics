#!/bin/bash

# --- CONFIGURATION ---
SRC_DIR=$(pwd)
PROD_DIR="$HOME/Files/C/production binaries/manjaro_system_metrics"
PLASMOID_DIR="$HOME/.local/share/plasma/plasmoids/com.rob.manjarometrics/contents/ui"
BIN_NAME="daemon"

echo "--- Holisitic Fix Installer ---"

# 1. COMPILE
echo "[1/4] Compiling Daemon (Single-Line JSON)..."
make clean > /dev/null 2>&1
make
if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed."
    exit 1
fi

# 2. DEPLOY BINARIES
echo "[2/4] Deploying to $PROD_DIR..."
mkdir -p "$PROD_DIR"
cp "$SRC_DIR/$BIN_NAME" "$PROD_DIR/"
cp "$SRC_DIR/metrics.conf" "$PROD_DIR/"

# 3. DEPLOY QML (Manually copy the Main.qml you just saved)
# (Assuming you overwrote Main.qml in your source folder first!)
if [ -f "Main.qml" ]; then
    echo "[3/4] Deploying QML..."
    cp "Main.qml" "$PLASMOID_DIR/main.qml"
else
    echo "WARNING: Main.qml not found in current folder. Skipping QML copy."
fi

# 4. RESTART
echo "[4/4] Restarting Stack..."
killall daemon > /dev/null 2>&1
killall plasmashell > /dev/null 2>&1
rm -r ~/.cache/plasma* > /dev/null 2>&1

cd "$PROD_DIR"
./daemon &
echo "Daemon started."

if command -v kstart6 >/dev/null 2>&1; then
    kstart6 plasmashell > /dev/null 2>&1 &
else
    plasmashell > /dev/null 2>&1 &
fi

echo "Done. Check the panel."
