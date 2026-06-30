//below code is for "EyeBlinkController.cs" , which is responsible for controlling the eye blinking of
//the character using blendshapes. It implements a natural blinking pattern by using separate timing for
//both-eye blinks (more frequent) and single-eye blinks (rare), without relying on an Animator component.
//The script continuously runs a blinking routine that randomly decides between both-eye and single-eye blinks,
//creating a more lifelike appearance during idle, talking, and emotional states.

using UnityEngine;
using System.Collections;

/// <summary>
/// EyeBlinkController
/// ------------------
/// Handles natural human-like eye blinking using blendshapes.
///
/// ADDITIONS MADE:
/// ✔ Separate timing for BOTH-eye blinks (more frequent)
/// ✔ Single-eye blinks remain rare
/// ✔ No Animator dependency
/// ✔ Works during idle, talking, emotions
/// </summary>
public class EyeBlinkController : MonoBehaviour
{
    // Face mesh that contains blink blendshapes
    public SkinnedMeshRenderer faceMesh;

    [Header("BlendShape Indexes (from SkinnedMeshRenderer)")]
    // These MUST match your blendshape order exactly
    public int blinkLeft = 9;
    public int blinkRight = 10;
    public int blinkBoth = 11;

    [Header("Blink Timing (seconds)")]

    // 🔹 NEW: Faster & more frequent BOTH-eye blinking
    // Human-like frequent blinking (especially while talking)
    public float minBothBlinkTime = 0.8f;
    public float maxBothBlinkTime = 2.0f;

    // 🔹 NEW: Slower & rare single-eye blinking
    // Prevents unnatural winking
    public float minSingleBlinkTime = 4.0f;
    public float maxSingleBlinkTime = 7.0f;

    [Header("Blink Speed")]
    // Lower value = slower & fuller blink (natural)
    public float bothEyeBlinkSpeed = 0.06f;

    // Higher value = faster blink (subtle single-eye wink)
    public float singleEyeBlinkSpeed = 0.12f;

    void Start()
    {
        // Start blinking loop once game starts
        StartCoroutine(BlinkRoutine());
    }

    /// <summary>
    /// Main blinking loop
    /// Uses different timing for both-eye and single-eye blinks
    /// </summary>
    IEnumerator BlinkRoutine()
    {
        while (true)
        {
            int blinkType = Random.Range(0, 10);

            // 👀 BOTH EYES (80% chance, frequent)
            if (blinkType < 8)
            {
                // Wait shorter time → more frequent blinking
                yield return new WaitForSeconds(
                    Random.Range(minBothBlinkTime, maxBothBlinkTime)
                );

                yield return StartCoroutine(
                    Blink(blinkBoth, bothEyeBlinkSpeed)
                );
            }
            // 👁 LEFT EYE (rare)
            else if (blinkType == 8)
            {
                yield return new WaitForSeconds(
                    Random.Range(minSingleBlinkTime, maxSingleBlinkTime)
                );

                yield return StartCoroutine(
                    Blink(blinkLeft, singleEyeBlinkSpeed)
                );
            }
            // 👁 RIGHT EYE (rare)
            else
            {
                yield return new WaitForSeconds(
                    Random.Range(minSingleBlinkTime, maxSingleBlinkTime)
                );

                yield return StartCoroutine(
                    Blink(blinkRight, singleEyeBlinkSpeed)
                );
            }
        }
    }

    /// <summary>
    /// Performs a smooth blink animation
    /// Speed is controlled per blink type
    /// </summary>
    IEnumerator Blink(int blendShapeIndex, float speed)
    {
        // CLOSE eye
        for (float t = 0; t <= 1; t += speed)
        {
            faceMesh.SetBlendShapeWeight(blendShapeIndex, t * 100f);
            yield return null;
        }

        // OPEN eye
        for (float t = 1; t >= 0; t -= speed)
        {
            faceMesh.SetBlendShapeWeight(blendShapeIndex, t * 100f);
            yield return null;
        }

        // Ensure eye is fully open at end
        faceMesh.SetBlendShapeWeight(blendShapeIndex, 0f);
    }
}
