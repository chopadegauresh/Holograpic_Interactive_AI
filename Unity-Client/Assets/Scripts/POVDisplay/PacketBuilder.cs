using System;

public static class PacketBuilder
{
    public static byte[] Build(byte[] imageData, ushort frameID)
    {
        ushort length = (ushort)imageData.Length;

        ushort crc = CRC16.Calculate(imageData);

        byte[] packet = new byte[length + 9];

        int index = 0;

        packet[index++] = 0xAA;
        packet[index++] = 0x55;

        Array.Copy(BitConverter.GetBytes(frameID), 0, packet, index, 2);
        index += 2;

        Array.Copy(BitConverter.GetBytes(length), 0, packet, index, 2);
        index += 2;

        Array.Copy(imageData, 0, packet, index, imageData.Length);
        index += imageData.Length;

        Array.Copy(BitConverter.GetBytes(crc), 0, packet, index, 2);
        index += 2;

        packet[index] = 0xFE;

        return packet;
    }
}