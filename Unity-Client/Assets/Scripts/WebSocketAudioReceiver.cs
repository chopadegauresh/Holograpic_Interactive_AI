using System;
using System.Collections.Generic;
using NativeWebSocket;
using UnityEngine;

public class WebSocketAudioReceiver : MonoBehaviour
{
    [Header("References")]
    public AICharacterBrain characterBrain;
    public AudioSource audioSource;

    private WebSocket ws;
    public bool IsReceivingVoice { get; private set; }

    // ===== Streaming Audio =====
    private Queue<float> audioQueue = new Queue<float>();
    private readonly object queueLock = new object();

    private const int SAMPLE_RATE = 22050;

    async void Start()
    {
        // Get AudioSource if not assigned
        if (audioSource == null)
            audioSource = GetComponent<AudioSource>();

        if (audioSource == null)
        {
            Debug.LogError("❌ AudioSource missing!");
            return;
        }

        // Recommended settings for OVR
        audioSource.playOnAwake = false;
        audioSource.loop = true;
        audioSource.spatialBlend = 0f; // 2D audio

        // ===== Create streaming AudioClip =====
        AudioClip clip = AudioClip.Create(
            "TTSStream",
            SAMPLE_RATE * 10,   // 10-second buffer
            1,                  // Mono
            SAMPLE_RATE,
            true,               // Streaming mode
            OnAudioRead         // PCM callback
        );

        audioSource.clip = clip;
        audioSource.Play();

        Debug.Log("🎧 Streaming AudioClip initialized");

        // ===== WebSocket Setup =====
        ws = new WebSocket("ws://127.0.0.1:8765");

        ws.OnOpen += () =>
        {
            Debug.Log("🔊 WebSocket connected");
        };

        ws.OnClose += (code) =>
        {
            Debug.Log("🔌 WebSocket disconnected");

            if (characterBrain != null)
                characterBrain.ForceIdle();
        };

        ws.OnError += (err) =>
        {
            Debug.LogError("WebSocket Error: " + err);
        };

        ws.OnMessage += (bytes) =>
        {
            // ---- Check for STATE message ----
            try
            {
                string message = System.Text.Encoding.UTF8.GetString(bytes);

                if (message.StartsWith("STATE:"))
                {
                    string state = message.Split(':')[1].Trim();

                    Debug.Log("📡 State received: " + state);

                    if (characterBrain != null)
                        characterBrain.HandleExternalState(state);

                    return;
                }
            }
            catch
            {
                // Not text → treat as audio
            }

            // ---- PCM Audio → Queue ----
            int sampleCount = bytes.Length / 2;

            if (sampleCount > 0)
                IsReceivingVoice = true;

            lock (queueLock)
            {
                for (int i = 0; i < sampleCount; i++)
                {
                    short s = BitConverter.ToInt16(bytes, i * 2);
                    float sample = s / 32768f;
                    audioQueue.Enqueue(sample);
                }
            }

        };

        await ws.Connect();
    }

    // ===== PCM Callback (Unity Audio Thread) =====
    void OnAudioRead(float[] data)
    {
        lock (queueLock)
        {
            for (int i = 0; i < data.Length; i++)
            {
                if (audioQueue.Count > 0)
                {
                    data[i] = audioQueue.Dequeue();
                }
                else
                {
                    data[i] = 0f;
                    IsReceivingVoice = false;
                }

            }
        }
    }

    void Update()
    {
#if !UNITY_WEBGL || UNITY_EDITOR
        ws?.DispatchMessageQueue();
#endif
    }

    async void OnDestroy()
    {
        if (ws != null)
            await ws.Close();
    }
}
