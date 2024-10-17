# Basic wrapper script to enable the web interface to be built before each compilation run

import subprocess

Import("env")

# Generate lib/web_ui.h by running the build_ui.sh script
result = subprocess.run(["./scripts/build_ui.sh"], shell=True, check=True)

if result.returncode != 0:
    print("UI Generation Failed!")
    env.Exit(1)
