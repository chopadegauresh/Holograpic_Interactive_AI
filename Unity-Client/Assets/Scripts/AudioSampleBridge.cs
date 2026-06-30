//below code is for "AudioSampleBridge.cs" , which serves as a bridge to receive audio samples from the

//Python WebSocket server and provide them to the SimpleLipSync script for analysis. It maintains a queue

//of recent audio samples and calculates the RMS (Root Mean Square) value to determine the volume level, which is used by the lip-syncing logic to control the character's mouth movements.



using System.Collections.Generic;

using UnityEngine;



public static class AudioSampleBridge

{

    static Queue<float> samples = new Queue<float>();

    const int MAX_SAMPLES = 1024;



    public static void Push(float sample)

    {

        samples.Enqueue(sample);

        if (samples.Count > MAX_SAMPLES)

            samples.Dequeue();

    }



    public static float GetRMS(int count)

    {

        if (samples.Count == 0)

            return 0f;



        int n = Mathf.Min(count, samples.Count);

        float sum = 0f;



        int i = 0;

        foreach (float s in samples)

        {

            sum += s * s;

            if (++i >= n) break;

        }



        return Mathf.Sqrt(sum / n);

    }

}

