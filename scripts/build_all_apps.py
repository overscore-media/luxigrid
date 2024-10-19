# _    _  _ _  _ _ ____ ____ _ ___
# |    |  |  \/  | | __ |__/ | |  \
# |___ |__| _/\_ | |__] |  \ | |__/
# =================================
# Luxigrid - All Apps Builder Script
# Copyright (c) 2024 OverScore Media - MIT License
#
# MIT LICENSE:
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Import("env")
import os
import shutil
import zipfile

dist_dir = os.path.join(os.getcwd(), "dist")
env_file_path = os.path.join(os.getcwd(), "ui", ".env")

apps = [
    # Apps with App-specific Config
    {"name": "GIF Player", "id": "gif-player", "define": "GIF_PLAYER", "config": True},
    {"name": "Morphing Clock", "id": "morphing-clock", "define": "MORPHING_CLOCK", "config": True},
    {"name": "Stock Ticker", "id": "stock-ticker", "define": "STOCK_TICKER", "config": True},
    {"name": "Pong Wars", "id": "pong-wars", "define": "PONG_WARS", "config": True},
    {"name": "Weather Station", "id": "weather-station", "define": "WEATHER_STATION", "config": True},

    # Apps without App-specific Config
    {"name": "Attract", "id": "attract", "define": "ATTRACT"},
    {"name": "Bubbles", "id": "bubbles", "define": "BUBBLES"},
    {"name": "Code Rain", "id": "code-rain", "define": "CODE_RAIN"},
    {"name": "DVD Logo", "id": "dvd-logo", "define": "DVD_LOGO"},
    {"name": "Electric Mandala", "id": "electric-mandala", "define": "ELECTRIC_MANDALA"},
    {"name": "Flock", "id": "flock", "define": "FLOCK"},
    {"name": "Flow Field", "id": "flow-field", "define": "FLOW_FIELD"},
    {"name": "Hue Value Spectrum", "id": "hue-value-spectrum", "define": "HUE_VALUE_SPECTRUM"},
    {"name": "Incremental Drift", "id": "incremental-drift", "define": "INCREMENTAL_DRIFT"},
    {"name": "Julia Set", "id": "julia-set", "define": "JULIA_SET"},
    {"name": "Life", "id": "life", "define": "LIFE"},
    {"name": "Maze", "id": "maze", "define": "MAZE"},
    {"name": "Munch", "id": "munch", "define": "MUNCH"},
    {"name": "Pendulum Wave", "id": "pendulum-wave", "define": "PENDULUM_WAVE"},
    {"name": "Periodic Table", "id": "periodic-table", "define": "PERIODIC_TABLE"},
    {"name": "Plasma", "id": "plasma", "define": "PLASMA"},
    {"name": "Pong Clock", "id": "pong-clock", "define": "PONG_CLOCK"},
    {"name": "Simplex Noise", "id": "simplex-noise", "define": "SIMPLEX_NOISE"},
    {"name": "Snake Game", "id": "snake-game", "define": "SNAKE_GAME"},
    {"name": "Snakes", "id": "snakes", "define": "SNAKES"},
    {"name": "Swirl", "id": "swirl", "define": "SWIRL"},
    {"name": "TV Test Pattern", "id": "tv-test-pattern", "define": "TV_TEST_PATTERN"},
]

for app in apps:
    app_name = app["name"]
    app_id = app["id"]
    app_define = app["define"]

    root_build_dir = env["BUILD_DIR"]

    # This sets the relevant #define for the app
    # When running this script, make sure that /lib/apps.h has no app #define's set
    os.environ["PLATFORMIO_BUILD_FLAGS"] = f"-D {app_define}"

    # Modify /ui/.env to build the proper web interface for this app
    try:
        if os.path.exists(env_file_path):
            with open(env_file_path, "r") as env_file:
                lines = env_file.readlines()
        else:
            lines = []

        updated_lines = []
        found_app_name_line = False

        for line in lines:
            if 'config' in app and app["config"] and line.startswith("VITE_CURRENT_APP="):
                updated_lines.append(f"VITE_CURRENT_APP={app_id}\n")
            elif line.startswith("VITE_APP_NAME="):
                updated_lines.append(f"VITE_APP_NAME={app_name}\n")
                found_app_name_line = True
            else:
                updated_lines.append(line)

        if not found_app_name_line:
            updated_lines.append(f"VITE_APP_NAME={app_name}\n")

        with open(env_file_path, "w") as env_file:
            env_file.writelines(updated_lines)

        print(f"Updated .env file for {app_name} with ID {app_id}")

    except Exception as e:
        print(f"Failed to update .env file: {e}")

    print(f"Building {app_name}...")
    env.Execute(f"pio run --silent -e esp32dev --project-dir {os.getcwd()}")

    # Locate the generated firmware file
    firmware_src = os.path.join(".pio", "build", "esp32dev", "firmware.bin")
    firmware_dst = os.path.join(dist_dir, f"{app_id}.bin")

    # Rename and move the firmware file
    if os.path.exists(firmware_src):
        shutil.move(firmware_src, firmware_dst)
        print(f"Moved firmware to {firmware_dst}")
    else:
        print(f"Firmware not found for {app_name} at {firmware_src}")

# Create a .zip file of all .bin files in the dist directory
zip_file_path = os.path.join(dist_dir, "firmware_bundle.zip")

with zipfile.ZipFile(zip_file_path, 'w') as zipf:
    for root, _, files in os.walk(dist_dir):
        for file in files:
            if file.endswith(".bin"):
                file_path = os.path.join(root, file)
                zipf.write(file_path, os.path.basename(file_path))

print(f"Created zip archive of firmware files at {zip_file_path}")