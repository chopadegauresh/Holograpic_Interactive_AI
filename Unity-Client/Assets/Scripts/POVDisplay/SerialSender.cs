using UnityEngine;
using System.IO.Ports;

public class SerialSender : MonoBehaviour
{
    [Header("Serial Settings")]
    public string portName = "COM5";
    public int baudRate = 3000000;

    [Header("References")]
    public POVFrameBuilder frameBuilder;

    private SerialPort serial;
    private ushort frameID = 0;

    void Start()
    {
        try
        {
            serial = new SerialPort(portName, baudRate);
            serial.WriteTimeout = 100;
            serial.Open();

            Debug.Log("Connected to " + portName);

            InvokeRepeating(nameof(SendFrame), 0f, 1f / 30f);
        }
        catch (System.Exception e)
        {
            Debug.LogError(e.Message);
        }
    }

    void SendFrame()
    {
        if (serial == null || !serial.IsOpen)
            return;

        // Capture latest frame
        Color32[] pixels = frameBuilder.GetFramePixels();

        if (pixels == null)
            return;

        // Convert RGB888 -> RGB565
        byte[] frame = RGB565Converter.Convert(pixels);

        // Build communication packet
        byte[] packet = PacketBuilder.Build(frame, frameID);

        // Send packet
        serial.Write(packet, 0, packet.Length);

        frameID++;
    }

    void OnApplicationQuit()
    {
        if (serial != null && serial.IsOpen)
            serial.Close();
    }
}