import os
import sys

Import("env")


def get_esp32_ip(source, target, env):
    esp32_ip = ""

    try:
        with open(".env", "r") as f:
            for line in f:
                if line.startswith("ESP32_IP="):
                    esp32_ip = line.split("=", 1)[1].strip()
    except:
        print("Please specify an ESP32_IP in the .env file")
        env.Exit(1)
    if esp32_ip == "" or esp32_ip == None:
        sys.stderr.write(
            "\n\nESP32 IP Address Not Specified!\n===============================\nPlease specify an ESP32_IP in the .env file, to enable OTA upload directly from PlatformIO\n\n\n"
        )
        env.Exit(1)
    else:
        env.Replace(IP=esp32_ip)


env.AddPreAction("upload", get_esp32_ip)
