from scapy import all as scapy

TYPE_PROBE = 0x819


class Records(scapy.Packet):
    fields_desc = [scapy.ByteField("records", 0)]


class RecordData(scapy.Packet):
    fields_desc = [scapy.ShortField("port", 0)]


scapy.bind_layers(scapy.Ether, Records, type=TYPE_PROBE)
scapy.bind_layers(Records, RecordData)
scapy.bind_layers(RecordData, RecordData)
