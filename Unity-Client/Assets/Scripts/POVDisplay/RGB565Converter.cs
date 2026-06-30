using UnityEngine;

public static class RGB565Converter
{
    public static byte[] Convert(Color32[] pixels)
    {
        byte[] rgb565 = new byte[pixels.Length * 2];

        int index = 0;

        for (int i = 0; i < pixels.Length; i++)
        {
            ushort color = (ushort)(
                ((pixels[i].r & 0xF8) << 8) |
                ((pixels[i].g & 0xFC) << 3) |
                (pixels[i].b >> 3));

            rgb565[index++] = (byte)(color >> 8);
            rgb565[index++] = (byte)(color & 0xFF);
        }

        return rgb565;
    }
}