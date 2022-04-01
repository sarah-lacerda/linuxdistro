import http.server
import os.path
import re
import cpustat as cpustat

htmlTemplate = ""
with open("index.html", "r") as htmlFile:
    htmlTemplate = htmlFile.read().replace("\n", "")


def getSystemInfo():
    dateTime = os.popen("date").read().split()
    date = dateTime[0] + " " + dateTime[1] + \
        " " + dateTime[2] + " " + dateTime[5]
    time = dateTime[3]

    with open('/proc/uptime') as f:
        uptime = f.read()
    uptime = uptime.split()[0]

    cpuinf_dict = {}
    with open('/proc/cpuinfo', mode='r') as cpuinfo:
        for line in cpuinfo:
            name, value = line.partition(":")[::2]
            cpuinf_dict[name.strip()] = value.strip()
    cpu = cpuinf_dict["model name"]

    cpuUsage = str(
        format((cpustat.GetCpuLoad().getcpuload()['cpu'] * 100), ".2f")) + " %"

    meminfo_dict = {}
    with open('/proc/meminfo', mode='r') as cpuinfo:
        for line in cpuinfo:
            name, value = line.partition(":")[::2]
            meminfo_dict[name.strip()] = re.sub("[^0-9]", "", value.strip())
    ram = str((int(meminfo_dict["MemTotal"])-int(meminfo_dict["MemFree"])) /
              1000) + "/" + str(int(meminfo_dict["MemTotal"])/1000) + " MB"

    with open('/proc/version') as f:
        version = f.read()

    return date, time, uptime, cpu, cpuUsage, ram, version, getProcessList()


def getProcessList():
    processes = "<p>"
    ps = os.popen("ps -Ao pid,comm").read()
    for i in range(len(ps)):
        if (ps[i] == "\n"):
            processes += "</p>\n"
        else:
            processes += ps[i]
    return processes


class MyHandler(http.server.BaseHTTPRequestHandler):
    getSystemInfo = getSystemInfo

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()

        message = htmlTemplate % (getSystemInfo())
        self.wfile.write(message.encode())
#        self.wfile.close()


Handler = MyHandler
server = http.server.HTTPServer(('0.0.0.0', 8080), Handler)
server.serve_forever()