//below code is for "SimpleLipSync.cs" , which is responsible for analyzing audio input (via AudioSampleBridge)
//to determine if the character is speaking and to control the blendshapes of the character's face for lip-syncing.
//It uses a simple RMS threshold to detect speech and applies a phoneme-style distribution of blendshape weights based
//on the detected audio level, creating a more natural lip movement effect.

using UnityEngine;

public class SimpleLipSync : MonoBehaviour
{
    [Header("References")]
    public SkinnedMeshRenderer faceMesh;

    [Header("BlendShape Indexes")]
    public int AI = 0;
    public int E = 1;
    public int O = 2;
    public int U = 3;
    public int CDGK = 4;
    public int FV = 5;
    public int L = 6;
    public int WQ = 7;
    public int Silence = 8;

    float jaw;
    float speakingHold;

    public bool IsSpeaking { get; private set; }

    void Start()
    {
        Debug.Log("SimpleLipSync initialized");
    }

    // =====================================
    // SPEECH DETECTION (Python audio based)
    // =====================================
    void Update()
    {
        // Get volume level from AudioSampleBridge
        float rms = AudioSampleBridge.GetRMS(256);

        // Threshold for detecting speech
        if (rms > 0.012f)
        {
            speakingHold = 0.1f;   // keep speaking state briefly
        }
        else
        {
            speakingHold -= Time.deltaTime;
        }

        IsSpeaking = speakingHold > 0f;
    }

    // =====================================
    // VISUAL LIP MOVEMENT
    // =====================================
    void LateUpdate()
    {
        float rms = AudioSampleBridge.GetRMS(256);

        float target = (rms < 0.004f)
            ? 0f
            : Mathf.Clamp(rms * 140f, 0f, 45f);

        // Faster open, slower close for natural feel
        float speed = (target > jaw) ? 14f : 32f;
        jaw = Mathf.Lerp(jaw, target, Time.deltaTime * speed);

        // Reset all blendshapes
        for (int i = 0; i <= 8; i++)
            faceMesh.SetBlendShapeWeight(i, 0f);

        // Apply phoneme-style distribution
        faceMesh.SetBlendShapeWeight(AI, jaw);
        faceMesh.SetBlendShapeWeight(E, jaw * 0.6f);
        faceMesh.SetBlendShapeWeight(O, jaw * 0.7f);
        faceMesh.SetBlendShapeWeight(U, jaw * 0.5f);
        faceMesh.SetBlendShapeWeight(CDGK, jaw * 0.4f);
        faceMesh.SetBlendShapeWeight(FV, jaw * 0.3f);
        faceMesh.SetBlendShapeWeight(L, jaw * 0.25f);
        faceMesh.SetBlendShapeWeight(WQ, jaw * 0.2f);
        faceMesh.SetBlendShapeWeight(Silence, 100f - jaw);
    }
}
