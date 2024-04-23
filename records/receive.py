from hdrs import RecordData, Records, scapy

def expand(x):
    yield x
    while x.payload:
        x = x.payload
        yield x


def handle_pkt(pkt):
    if RecordData in pkt:
        data_layers = [l for l in expand(pkt) if l.name == "RecordData"]
        print("")
        for sw in data_layers:
            print(sw.port)


def main():
    iface = "eth0"
    print(f"sniffing on {iface}")
    scapy.sniff(iface=iface, prn=lambda x: handle_pkt(x))


if __name__ == "__main__":
    main()