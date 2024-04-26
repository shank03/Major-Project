import time
import subprocess

pid_map = {}
switches = ["s0", "s1", "s2", "s3", "s4"]
for s in switches:
    ps = subprocess.Popen(
        "ps aux | grep %s | grep stratum | awk '{print $2}'" % s,
        stdout=subprocess.PIPE,
        shell=True,
    )
    (pOp, e) = ps.communicate()
    ps.wait()

    pid_map[s] = {"pid": str(pOp).split("\n")[0], "cpu": 0}

print("PIDs: ", pid_map)

while True:
    try:
        for k, v in pid_map.items():
            proc = subprocess.Popen(
                "top -b -n 1 -p %s | grep %s | awk '{print $9}'" % (v["pid"], v["pid"]),
                stdout=subprocess.PIPE,
                shell=True,
            )
            (op, err) = proc.communicate()
            proc.wait()

            cpu = str(op).split("\n")[0]
            try:
                pid_map[k]["cpu"] = int(round(float(cpu)))
            except:
                pid_map[k]["cpu"] = 0

        f = open("/records/top.csv", "w")
        for s in switches:
            f.write(
                "device:"
                + s
                + ","
                + str(pid_map[s]["pid"])
                + ","
                + str(pid_map[s]["cpu"])
                + "\n"
            )

        f.close()
        time.sleep(0.2)
    except KeyboardInterrupt:
        exit(0)
