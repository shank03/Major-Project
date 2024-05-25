import sys

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
        record = [l for l in expand(pkt) if l.name == "Records"][0]
        print("records: %s, eth_type: %s" % (record.records, record.ether_type))
        for sw in data_layers:
            print("dpid: %s, cpu: %s, timestamp: %s" % (sw.dpid, sw.cpu, sw.timestamp))


def main():
    iface = sys.argv[1]
    print("sniffing on %s" % iface)
    scapy.sniff(iface=iface, prn=lambda x: handle_pkt(x))


if __name__ == "__main__":
    main()
