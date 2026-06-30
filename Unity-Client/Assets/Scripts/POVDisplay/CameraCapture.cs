using UnityEngine;

public class CameraCapture : MonoBehaviour
{
    [Header("Camera")]
    public Camera renderCamera;

    [Header("Render Texture")]
    public RenderTexture renderTexture;

    [Header("Capture Size")]
    public int captureWidth = 360;
    public int captureHeight = 36;

    private Texture2D frameTexture;

    public Texture2D CurrentFrame
    {
        get { return frameTexture; }
    }

    void Start()
    {
        renderCamera.targetTexture = renderTexture;

        frameTexture = new Texture2D(
            captureWidth,
            captureHeight,
            TextureFormat.RGB24,
            false
        );
    }

    public void Capture()
    {
        RenderTexture.active = renderTexture;

        frameTexture.ReadPixels(
            new Rect(0, 0, captureWidth, captureHeight),
            0,
            0
        );

        frameTexture.Apply();

        RenderTexture.active = null;
    }
}