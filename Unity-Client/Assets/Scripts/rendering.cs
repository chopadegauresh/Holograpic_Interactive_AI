using UnityEngine;
using System.IO;

public class FrameRenderer : MonoBehaviour
{
    public Camera renderCamera;
    public RenderTexture renderTexture;

    public int width = 512;
    public int height = 512;

    private Texture2D frameTexture;

    void Start()
    {
        // Assign Render Texture to Camera
        renderCamera.targetTexture = renderTexture;

        // Texture for frame capture
        frameTexture = new Texture2D(width, height, TextureFormat.RGB24, false);

        // Create folder
        if (!Directory.Exists(Application.dataPath + "/Frames"))
        {
            Directory.CreateDirectory(Application.dataPath + "/Frames");
        }
    }

    void Update()
    {
        CaptureFrame();
    }

    void CaptureFrame()
    {
        // Set active render texture
        RenderTexture.active = renderTexture;

        // Read pixels from camera
        frameTexture.ReadPixels(new Rect(0, 0, width, height), 0, 0);
        frameTexture.Apply();

        // Convert to PNG
        byte[] bytes = frameTexture.EncodeToPNG();

        // Save frame
        string fileName = "frame_" + Time.frameCount + ".png";

        File.WriteAllBytes(
            Application.dataPath + "/Frames/" + fileName,
            bytes
        );

        Debug.Log("Frame Saved: " + fileName);

        // Reset
        RenderTexture.active = null;
    }
}