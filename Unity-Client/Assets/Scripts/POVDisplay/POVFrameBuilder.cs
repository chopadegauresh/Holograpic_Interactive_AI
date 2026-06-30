using UnityEngine;

public class POVFrameBuilder : MonoBehaviour
{
    [Header("Reference")]
    public CameraCapture cameraCapture;

    public Color32[] GetFramePixels()
    {
        if (cameraCapture == null)
        {
            Debug.LogError("CameraCapture not assigned!");
            return null;
        }

        // Capture latest frame
        cameraCapture.Capture();

        // Return pixels in row-major order
        return cameraCapture.CurrentFrame.GetPixels32();
    }
}