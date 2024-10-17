#!/bin/bash

# This script generates the lib/web_ui.h file from the frontend code in the ui directory
# web_ui.h contains a gzipped binary representation of the frontend code that the web server will host

# This script must be run to generate a new web_ui.h whenever files in the ui directory have been updated
# It is automatically run when compiling with PlatformIO - see platformio.ini
echo -e "_    _  _ _  _ _ ____ ____ _ ___  \n|    |  |  \/  | | __ |__/ | |  \ \n|___ |__| _/\_ | |__] |  \ | |__/ \n"
echo -e "==== LUXIGRID UI BUILD SCRIPT ====\n"

cd ui

echo -e "Generating Web UI...\n"
npm run build > /dev/null 2>&1

if [ $? -ne 0 ]; then
  echo "Failed to Generate Web UI"
  exit 1
fi

cp dist/html.h ../lib/web_ui.h
cd ..
echo -e "Web UI Generated Successfully!\n"