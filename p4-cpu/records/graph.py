import pandas as pd
from matplotlib import pyplot as plt

df = pd.read_csv("records/cpu.csv", names=["Switch", "CPU", "Timestamp"])
print("CSV contents: ", df)

plt.figure(figsize=(10, 6))

for switch, data in df.groupby("Switch"):
    plt.plot(data["Timestamp"], data["CPU"], linestyle='solid', marker='o', label=switch)

plt.xlabel("Time")
plt.ylabel("CPU Usage")
plt.legend()
plt.grid(True)
plt.savefig("records/cpu.png")
