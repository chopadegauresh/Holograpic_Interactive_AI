# =========================================================
#         STABLE ALEXA-LIKE ALL-IN-ONE VOICE ASSISTANT
# =========================================================

import os, time, json, queue, subprocess, requests, threading
import numpy as np
import sounddevice as sd
import soundfile as sf
import pvporcupine
from pvrecorder import PvRecorder
from vosk import Model, KaldiRecognizer
from llama_cpp import Llama
import psutil
print("Default devices:", sd.default.device)

# ================= CONFIG =================
ACCESS_KEY = "HaAyTSoHm4saCsEHj2lzL6E+ftm+2FjAyjP3zWt1RHxAOAUuJSe5JA=="
WAKE_WORD = "hey siri"

VOSK_MODEL_DIR = "vosk-model-small-en-us-0.15"
PIPER_MODEL = "piper_voices/en_US-libritts_r-medium.onnx"
PHI2_MODEL_PATH = "models/phi2.gguf"

FOLLOWUP_TIMEOUT = 15
MIN_WORDS = 1 
SILENCE_END = 2.0 

GROQ_API_KEY = os.getenv("GROQ_API_KEY")
GROQ_URL = "https://api.groq.com/openai/v1/chat/completions"
GROQ_MODEL = "llama-3.1-8b-instant"

SAMPLE_RATE = 16000
DEVICE_INDEX = -1 
# =========================================

# ================= STATE ==================
audio_q = queue.Queue()
speech_q = queue.Queue()  # NEW: Prevents double/overlapping sound
stop_event = threading.Event()

MODE = "WAKE" 
SPEAKING = False 
audio_detected = False 
conversation_history = [] 
MAX_HISTORY = 20 
# =========================================






#below function checks for internet connectivity by going to google 
# if not than returns false and goes to offline mode
#generate_204 is a special URL that google uses to check for connectivity for faster response
# ================= UTIL ===================
def internet_available():
    try:
        requests.get("https://www.google.com/generate_204", timeout=2)
        return True
    except:
        return False

# ================= 🛠️ LOCAL SKILLS =================
def run_skill(query):
    """Checks for simple local commands to respond instantly."""
    q = query.lower()

    # ---- TIME & DATE ----
    if "time" in q:
        return f"The time is {time.strftime('%I:%M %p')}."

    if "date" in q:
        return f"Today is {time.strftime('%A, %B %d')}."

    # ---- GREETINGS ----
    if "how are you" in q:
        return "I am doing great, thank you for asking!"

    # ---- SYSTEM STATUS ----
    if "battery" in q:
        return get_battery_status()

    if "cpu" in q:
        return get_cpu_status()

    if "ram" in q or "memory" in q:
        return get_ram_status()

    # ---- SYSTEM CONTROL ----
    if "shutdown" in q:
        os.system("shutdown /s /t 5")
        return "Shutting down the system in 5 seconds."

    if "restart" in q:
        os.system("shutdown /r /t 5")
        return "Restarting the system in 5 seconds."

    # ---- IOT / GPIO ----
    if "turn on motor" in q:
        gpio_on()
        return "Motor turned on."

    if "turn off motor" in q:
        gpio_off()
        return "Motor turned off."

   
   
    # ---- NO MATCH ----
    return None




#this function gets battery status using psutil library and also checks if the battery is charging or not
def get_battery_status():
    battery = psutil.sensors_battery()
    if battery is None:
        return "Battery information is not available."

    percent = int(battery.percent)
    if battery.power_plugged:
        return f"Battery is {percent} percent and charging."
    else:
        return f"Battery is {percent} percent."
#this function gets cpu usage using psutil library 
def get_cpu_status():
    return f"CPU usage is {psutil.cpu_percent(interval=0.5)} percent."
#this function gets ram usage using psutil library 
def get_ram_status():
    ram = psutil.virtual_memory()
    return f"RAM usage is {int(ram.percent)} percent."


# ================= OFFLINE AI ==============
llm = Llama(
    model_path=PHI2_MODEL_PATH,
    n_ctx=2048,
    n_threads=6,
    n_batch=256,
    verbose=False
)

SYSTEM_PROMPT = (
    "You are Alexa, a helpful and friendly voice assistant. "
    "Give short, clear, accurate answers. Be conversational but concise."
)

def ask_offline(prompt):
    full = f"<|system|>{SYSTEM_PROMPT}<|user|>{prompt}<|assistant|>"
    out = ""
    for chunk in llm(full, max_tokens=300, stream=True, temperature=0.4, stop=["<|user|>"]):
        text_chunk = chunk["choices"][0]["text"]
        out += text_chunk
    return out.strip()

# ================= ONLINE AI (STREAMING) ===============
def ask_online_stream(prompt, history=None):
    if not GROQ_API_KEY:
        raise RuntimeError("GROQ_API_KEY not set")

    if history is None:
        history = []

    messages = [
        {"role": "system", "content": SYSTEM_PROMPT}
    ] + history + [
        {"role": "user", "content": prompt}
    ]

    headers = {
        "Authorization": f"Bearer {GROQ_API_KEY}",
        "Content-Type": "application/json"
    }
    payload = {
        "model": GROQ_MODEL,
        "messages": messages,
        "temperature": 0.4,
        "max_tokens": 300,
        "stream": True 
    }
    
    r = requests.post(GROQ_URL, headers=headers, json=payload, timeout=8, stream=True)
    
    full_response = ""
    sentence_buffer = ""
    
    for line in r.iter_lines():
        if line:
            decoded_line = line.decode('utf-8').replace('data: ', '')
            if decoded_line == '[DONE]': break
            try:
                json_line = json.loads(decoded_line)
                content = json_line['choices'][0]['delta'].get('content', '')
                if content:
                    full_response += content
                    sentence_buffer += content
                    # Trigger speech as soon as a sentence ends
                    if any(punctuation in sentence_buffer for punctuation in ['.', '!', '?']):
                        speak(sentence_buffer.strip())
                        sentence_buffer = ""
            except:
                continue
                
    if sentence_buffer.strip():
        speak(sentence_buffer.strip())
        
    return full_response.strip()

# ================= ROUTER =================
def ask_brain(prompt, history=None):
    if history is None:
        history = []
    
    # 1. Check Local Skills First
    skill_check = run_skill(prompt)
    if skill_check:
        print("🛠️ LOCAL SKILL TRIGGERED")
        speak(skill_check)
        return skill_check

    # 2. Try Online Streaming
    if internet_available() and GROQ_API_KEY:
        try:
            print("🌐 USING ONLINE AI (STREAMING)")
            return ask_online_stream(prompt, history)
        except Exception as e:
            print(f"Online error: {e}")
            pass
    
    # 3. Fallback to Offline
    print("📴 USING OFFLINE AI")
    response = ask_offline(prompt)
    speak(response)
    return response

# ================= STT ====================
def stt_callback(indata, frames, time_info, status):
    global audio_detected
    audio_q.put((indata[:, 0] * 10 * 32767).astype(np.int16).tobytes())
    if np.max(np.abs(indata)) > 0.001 and not audio_detected:
        audio_detected = True

vosk_model = Model(VOSK_MODEL_DIR)

def listen_sentence(timeout):
    global SPEAKING, audio_detected
    recognizer = KaldiRecognizer(vosk_model, SAMPLE_RATE)
    audio_q.queue.clear()

    last_voice = time.time()
    start = time.time()
    audio_detected = False

    print(f"🎤 Listening...")
    try:
        with sd.InputStream(samplerate=SAMPLE_RATE, channels=1, callback=stt_callback):
            while time.time() - start < timeout:
                if not audio_q.empty():
                    data = audio_q.get()

                    if recognizer.AcceptWaveform(data):
                        text = json.loads(recognizer.Result()).get("text", "").lower()
                        if len(text.split()) >= MIN_WORDS:
                            return text

                    # VAD INTERRUPT LOGIC
                    partial = json.loads(recognizer.PartialResult()).get("partial", "").lower()
                    if partial:
                        last_voice = time.time()
                        if SPEAKING and ("stop" in partial or "quiet" in partial or "shut up" in partial or "no" in partial or "cancel" in partial):
                            # Clear the queue and stop current sound
                            stop_event.set()
                            while not speech_q.empty():
                                try: speech_q.get_nowait()
                                except: break
                            sd.stop()
                            SPEAKING = False
                            print("🛑 Interrupt: Voice command detected while speaking.")

                    if time.time() - last_voice > SILENCE_END:
                        final = json.loads(recognizer.FinalResult()).get("text", "").lower()
                        if len(final.split()) >= MIN_WORDS:
                            return final
                        return ""
    except Exception as e:
        print(f"🎤 Audio stream error: {e}")
    return ""

# ================= STREAMING TTS (FIXED QUEUE) ====================
def speech_worker():
    """Background thread that plays sentences one by one to prevent overlapping."""
    global SPEAKING
    while True:
        text = speech_q.get()
        if text is None: break
        
        stop_event.clear()
        SPEAKING = True
        command = ["piper", "--model", PIPER_MODEL, "--output_raw"]
        
        try:
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            process.stdin.write(text.encode())
            process.stdin.close()
            
            with sd.RawOutputStream(samplerate=22050, blocksize=1024, channels=1, dtype='int16') as stream:
                while True:
                    if stop_event.is_set():
                        process.terminate()
                        break
                    audio_chunk = process.stdout.read(1024)
                    if not audio_chunk: break
                    stream.write(audio_chunk)
            process.wait()
        except:
            pass
        finally:
            SPEAKING = False
            speech_q.task_done()

# Start the speech worker thread immediately
threading.Thread(target=speech_worker, daemon=True).start()

def speak(text):
    """Adds text to the queue for sequential playback."""
    if text:
        speech_q.put(text)

# ================= WAKE WORD ===============
porcupine = pvporcupine.create(access_key=ACCESS_KEY, keywords=[WAKE_WORD])
recorder = PvRecorder(device_index=DEVICE_INDEX, frame_length=porcupine.frame_length)
recorder.start()
print("🟢 Assistant running. Say:", WAKE_WORD)

# ================= MAIN LOOP ===============
try:
    while True:

        # 🔹 WAKE MODE
        if MODE == "WAKE":
            pcm = recorder.read()
            if porcupine.process(pcm) >= 0:
                MODE = "ACTIVE"
                print("👂 Wake word detected")
                time.sleep(0.3)
                speak("Yes? siri here")
                conversation_history.clear()

        # 🔹 ACTIVE MODE
        if MODE == "ACTIVE":
            while SPEAKING or not speech_q.empty(): # Wait for all speech to finish
                time.sleep(0.1)
            query = listen_sentence(8)
            if not query:
                speak("I did not hear anything")
                MODE = "WAKE"
                continue

            print("🗣️ You said:", query)
            response = ask_brain(query, conversation_history)
            
            # Update history
            conversation_history.append({"role": "user", "content": query})
            conversation_history.append({"role": "assistant", "content": response})
            if len(conversation_history) > MAX_HISTORY:
                conversation_history = conversation_history[-MAX_HISTORY:]

            MODE = "FOLLOWUP"
            follow_start = time.time()

        # 🔹 FOLLOW-UP MODE
        if MODE == "FOLLOWUP":
            if time.time() - follow_start > FOLLOWUP_TIMEOUT:
                MODE = "WAKE"
                continue
            
            while SPEAKING or not speech_q.empty(): # Wait for all speech to finish
                time.sleep(0.1)
            
            follow = listen_sentence(6)
            if follow:
                print("🗣️ Follow-up:", follow)
                if any(stop_word in follow for stop_word in ["thank you", "bye", "stop", "nevermind"]):
                    speak("You're welcome. Goodbye!")
                    MODE = "WAKE"
                    continue
                    
                response = ask_brain(follow, conversation_history)
                
                conversation_history.append({"role": "user", "content": follow})
                conversation_history.append({"role": "assistant", "content": response})
                follow_start = time.time()

except KeyboardInterrupt:
    pass
finally:
    recorder.stop()
    recorder.delete()
    porcupine.delete()