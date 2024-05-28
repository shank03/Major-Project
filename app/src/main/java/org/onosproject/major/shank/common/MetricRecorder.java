package org.onosproject.major.shank.common;

import org.apache.commons.lang3.tuple.Pair;
import org.onlab.packet.Ethernet;
import org.onlab.packet.MacAddress;
import org.onosproject.major.shank.headers.RecordDataHeader;
import org.onosproject.major.shank.headers.RecordHeader;
import org.onosproject.net.DeviceId;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class MetricRecorder {

    private MetricRecorder() {
    }

    public static class Record {
        private DeviceId provider;
        private MacAddress dpid;
        private int cpu;
        private long timestamp;

        public Record(DeviceId provider, MacAddress dpid, int cpu, long timestamp) {
            this.provider = provider;
            this.dpid = dpid;
            this.cpu = cpu;
            this.timestamp = timestamp;
        }

        public MacAddress getDpid() {
            return dpid;
        }

        public int getCpu() {
            return cpu;
        }

        public long getTimestamp() {
            return timestamp;
        }

        public DeviceId getProvider() {
            return provider;
        }
    }

    private static final HashMap<Pair<MacAddress, MacAddress>, List<Record>> dbRecords = new HashMap<>();

    public static HashMap<Pair<MacAddress, MacAddress>, List<Record>> getDbRecords() {
        return dbRecords;
    }

    public static void extractRecords(MacAddress srcMac, MacAddress dstMac, byte[] payloadBytes, DeviceId deviceId) {
        try {

            Pair<MacAddress, MacAddress> key = Pair.of(srcMac, dstMac);

            RecordHeader records = RecordHeader.deserializer().deserialize(
                    payloadBytes,
                    Ethernet.ETHERNET_HEADER_LENGTH,
                    RecordHeader.RECORD_HEADER_LENGTH
            );
            System.out.println("device: " + deviceId
                    + "; recs: " + records.getNumberOfRecords()
                    + "; eth_type: " + String.format("0x%s", Integer.toHexString(records.getEthType() & 0xffff)));
            for (int i = 0; i < records.getNumberOfRecords(); i++) {
                RecordDataHeader data = RecordDataHeader.deserializer()
                        .deserialize(payloadBytes,
                                Ethernet.ETHERNET_HEADER_LENGTH + RecordHeader.RECORD_HEADER_LENGTH + (RecordDataHeader.RECORD_DATA_HEADER_LENGTH * i),
                                RecordDataHeader.RECORD_DATA_HEADER_LENGTH
                        );
                System.out.println("dpid: " + data.getDpid() + ", cpu: " + data.getCpuUsage() + ", timestamp: " + data.getTimestamp());

                List<Record> list = dbRecords.getOrDefault(key, new ArrayList<>());
                list.add(new Record(deviceId, data.getDpid(), data.getCpuUsage(), data.getTimestamp()));
                dbRecords.put(key, list);
            }
        } catch (Exception e) {
            System.out.println("ERR: " + e.getMessage());
//            e.printStackTrace();
        }
    }
}
