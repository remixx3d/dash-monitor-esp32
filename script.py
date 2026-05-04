import psutil
import socket
import time
import subprocess
import json
import traceback
import os

print("starting")

ESP32_IP_WEB = "ip of esp32 which will be hosting the website"

PORT = 4210

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

psutil.cpu_percent(interval=None)

def system_uptime():
    try:
        with open("/proc/uptime", "r") as f:
            return float(f.readline().split()[0])
    except:
        return 0

def get_temp():
    try:
        out = subprocess.check_output(["sensors"], text=True)
        for line in out.splitlines():
            if "Tctl" in line or "Package id 0" in line:
                return float(line.split("+")[1].split("°")[0])
    except:
        pass
    return 0

def cpu_freq():
    try:
        f = psutil.cpu_freq()
        return f.current if f else 0
    except:
        return 0

def is_up(url):
    try:
        import requests
        r = requests.get(url, timeout=2)
        return r.status_code < 500
    except:
        return False

def stable(name, url):
    ok = is_up(url)
    return ok

while True:
    try:
        cpu = psutil.cpu_percent(interval=0.5)
        ram = psutil.virtual_memory()
        disk = psutil.disk_usage("/")

        services = {
            "jellyfin": stable("service name", "service address"),
            "radarr": stable("service name", "service address"),
            "sonarr": stable("service name", "service address"),
        }

        data = {
            "cpu": cpu,
            "temp": get_temp(),
            "cpu_freq": cpu_freq(),

            "ram_used": round(ram.used / (1024 ** 3), 2),
            "ram_total": round(ram.total / (1024 ** 3), 2),

            "disk_used": round(disk.used / (1024 ** 3), 2),
            "disk_total": round(disk.total / (1024 ** 3), 2),

            # 🔥 SYSTEM UPTIME
            "uptime": int(system_uptime()),

            "services": services
        }

        payload = json.dumps(data).encode()

        sock.sendto(payload, (ESP32_IP_WEB, PORT))
        print("sent:", data)

    except:
        print(traceback.format_exc())

    time.sleep(3)
