using UnityEngine;

public class AICharacterBrain : MonoBehaviour
{
    // ==============================
    // Core References
    // ==============================
    [Header("Core References")]
    public Animator animator;
    public AudioSource audioSource;
    public WebSocketAudioReceiver audioReceiver;

    private bool isInTalkingState = false;
    private bool isInListeningState = false;

    // ==============================
    // Movement Speeds
    // ==============================
    [Header("Movement Speeds")]
    public float idleSpeed = 1f;
    public float talkingSpeed = 1f;

    // Emotion Speeds
    public float hipHopSpeed = 1.2f;
    public float angrySpeed = 1.3f;
    public float stretchingSpeed = 1.1f;

    // ==============================
    // Emotion Animation Clips
    // ==============================
    [Header("Emotion Animation Clips")]
    public AnimationClip idleClip;
    public AnimationClip hipHopClip;
    public AnimationClip angryClip;
    public AnimationClip stretchingClip;
    // ==============================
    // Listening Timeout
    // ==============================
    [Header("Timeout Settings")]
    public float maxListeningTime = 5.0f;
    private float listeningTimer = 0f;

    // ==============================
    // Idle Emotion Weights
    // ==============================
    [Header("Idle Emotion Weights")]
    [Range(0, 100)] public int idleWeight = 60;
    [Range(0, 100)] public int hipHopWeight = 25;
    [Range(0, 100)] public int angryWeight = 15;
    [Range(0, 100)] public int stretchingWeight = 20;

    [Header("Idle Emotion Settings")]
    public float emotionChangeInterval = 5f;
    private float emotionTimer = 0f;

    // ==============================
    // Runtime Emotion Control
    // ==============================
    private float currentEmotionTime = 0f;
    private float currentEmotionDuration = 0f;
    private bool isEmotionPlaying = false;
    private int currentEmotion = 0; // 0 Idle, 1 HipHop, 2 Angry

    // ==============================
    // UPDATE
    // ==============================
    void Update()
    {
        if (animator == null || audioReceiver == null)
            return;

        bool speakingNow = audioReceiver.IsReceivingVoice;

        // ==============================
        // TALKING STATE
        // ==============================
        if (speakingNow)
        {
            listeningTimer = 0f;
            emotionTimer = 0f;
            isEmotionPlaying = false;
            currentEmotion = 0;

            animator.SetInteger("emotion", 0);

            if (!isInTalkingState)
            {
                isInTalkingState = true;
                isInListeningState = false;

                animator.SetBool("isTalking", true);
                animator.SetBool("isListening", false);
                animator.speed = talkingSpeed;

                Debug.Log("➡ STATE: TALKING");
            }
        }
        else
        {
            // Talking → Listening
            if (isInTalkingState)
            {
                isInTalkingState = false;
                isInListeningState = true;

                animator.SetBool("isTalking", false);
                animator.SetBool("isListening", true);
                animator.speed = idleSpeed;

                Debug.Log("👂 STATE: LISTENING");
            }

            // Listening timeout → Idle
            if (isInListeningState)
            {
                listeningTimer += Time.deltaTime;

                if (listeningTimer >= maxListeningTime)
                {
                    GoToIdle();
                }
            }
        }

        // ==============================
        // IDLE EMOTION SYSTEM
        // ==============================
        if (!isInTalkingState && !isInListeningState)
        {
            if (isEmotionPlaying)
            {
                currentEmotionTime += Time.deltaTime;

                if (currentEmotionTime >= currentEmotionDuration)
                {
                    animator.SetInteger("emotion", 0);
                    animator.speed = idleSpeed;

                    isEmotionPlaying = false;
                    currentEmotion = 0;
                    emotionTimer = 0f;

                    Debug.Log("🎭 Emotion finished → Idle");
                }
            }
            else
            {
                emotionTimer += Time.deltaTime;

                if (emotionTimer >= emotionChangeInterval && !isEmotionPlaying)
                {
                    emotionTimer = 0f;
                    ChooseRandomIdleEmotion();
                }
            }
        }
    }

    // ==============================
    // Duration Calculation
    // ==============================
    float GetAdjustedDuration(AnimationClip clip, float speed)
    {
        if (clip == null || speed <= 0f)
            return 0f;

        return clip.length / speed;
    }

    // ==============================
    // Weighted Random Emotion
    // ==============================
    void ChooseRandomIdleEmotion()
    {
        int total = idleWeight + hipHopWeight + angryWeight + stretchingWeight;
        if (total <= 0) return;

        int randomValue = Random.Range(0, total);
        int selectedEmotion;

        if (randomValue < idleWeight)
            selectedEmotion = 0;
        else if (randomValue < idleWeight + hipHopWeight)
            selectedEmotion = 1;
        else if (randomValue < idleWeight + hipHopWeight + angryWeight)
            selectedEmotion = 2;
        else
            selectedEmotion = 3;

        currentEmotion = selectedEmotion;
        currentEmotionTime = 0f;

        if (selectedEmotion == 0)
        {
            animator.SetInteger("emotion", 0);
            animator.speed = idleSpeed;

            currentEmotionDuration = GetAdjustedDuration(idleClip, idleSpeed);
            currentEmotionTime = 0f;
            isEmotionPlaying = true;

            Debug.Log("😴 Emotion: Idle | Duration: " + currentEmotionDuration);
        }
        else if (selectedEmotion == 1)
        {
            animator.SetInteger("emotion", 1);
            animator.speed = hipHopSpeed;

            currentEmotionDuration = GetAdjustedDuration(hipHopClip, hipHopSpeed);
            isEmotionPlaying = true;

            Debug.Log("🕺 Emotion: HipHop | Duration: " + currentEmotionDuration);
        }
        else if (selectedEmotion == 2)
        {
            animator.SetInteger("emotion", 2);
            animator.speed = angrySpeed;

            currentEmotionDuration = GetAdjustedDuration(angryClip, angrySpeed);
            isEmotionPlaying = true;

            Debug.Log("😡 Emotion: Angry | Duration: " + currentEmotionDuration);
        }
        else //(selectedEmotion == 3)
        {
            animator.SetInteger("emotion", 3);
            animator.speed = stretchingSpeed;

            currentEmotionDuration = GetAdjustedDuration(stretchingClip, stretchingSpeed);
            isEmotionPlaying = true;

            Debug.Log("🧘 Emotion: Stretching | Duration: " + currentEmotionDuration);
        }

    }

    // ==============================
    // External Commands
    // ==============================
    public void HandleExternalState(string state)
    {
        Debug.Log("📡 State: " + state);

        if (state == "LISTENING")
        {
            isInListeningState = true;
            isInTalkingState = false;

            animator.SetBool("isTalking", false);
            animator.SetBool("isListening", true);
        }
        else if (state == "IDLE")
        {
            GoToIdle();
        }
    }

    // ==============================
    // Idle State
    // ==============================
    public void GoToIdle()
    {
        isInTalkingState = false;
        isInListeningState = false;
        listeningTimer = 0f;

        animator.SetBool("isTalking", false);
        animator.SetBool("isListening", false);
        animator.SetInteger("emotion", 0);
        animator.speed = idleSpeed;

        emotionTimer = 0f;
        isEmotionPlaying = false;
        currentEmotion = 0;

        Debug.Log("😴 STATE: IDLE");
    }

    public void ForceIdle()
    {
        GoToIdle();
        Debug.Log("💤 FORCED IDLE");
    }
}
