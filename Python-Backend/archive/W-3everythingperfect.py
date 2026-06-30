# =========================================================
#         OPTIMIZED VOICE ASSISTANT WITH INTERRUPT
# =========================================================

import os, time, json, queue, subprocess, requests, threading
import numpy as np
import sounddevice as sd
import pvporcupine
from pvrecorder import PvRecorder
from vosk import Model, KaldiRecognizer
from llama_cpp import Llama
import psutil
from pycaw.pycaw import AudioUtilities


def set_volume_mute(mute=True):
    device = AudioUtilities.GetSpeakers()
    volume = device.EndpointVolume  # ✅ correct attribute
    volume.SetMute(int(mute), None)

def set_volume_level(level):
    """
    level: float between 0.0 and 1.0
    """
    level = max(0.0, min(1.0, level))  # clamp safety
    device = AudioUtilities.GetSpeakers()
    volume = device.EndpointVolume
    volume.SetMasterVolumeLevelScalar(level, None)

def parse_volume_percentage(text):
    text = text.lower()

    # Explicit keywords
    if "max" in text or "maximum" in text or "full" in text:
        return 1.0
    if "min" in text or "minimum" in text or "mute" in text:
        return 0.0
    if "hundred" in text:
        return 1.0

    words_to_numbers = {
        "zero": 0, "ten": 10, "twenty": 20, "thirty": 30,
        "forty": 40, "fifty": 50, "sixty": 60,
        "seventy": 70, "eighty": 80, "ninety": 90
    }

    for word, value in words_to_numbers.items():
        if word in text:
            return value / 100.0

    # numeric fallback (e.g. "volume 75")
    for token in text.split():
        if token.isdigit():
            v = int(token)
            if 0 <= v <= 100:
                return v / 100.0

    return None


# ================= CONFIG =================
ACCESS_KEY = "HaAyTSoHm4saCsEHj2lzL6E+ftm+2FjAyjP3zWt1RHxAOAUuJSe5JA=="
WAKE_WORD = "hey siri"
VOSK_MODEL_DIR = "vosk-model-small-en-us-0.15"
PIPER_MODEL = "piper_voices/en_US-libritts_r-medium.onnx"
PHI2_MODEL_PATH = "models/phi2.gguf"

FOLLOWUP_TIMEOUT = 15
MIN_WORDS = 1 
SILENCE_END = 2.0 
SAMPLE_RATE = 16000
DEVICE_INDEX = -1 

GROQ_API_KEY = os.getenv("GROQ_API_KEY")
GROQ_URL = "https://api.groq.com/openai/v1/chat/completions"
GROQ_MODEL = "llama-3.1-8b-instant"

# ================= STATE ==================
audio_q = queue.Queue()
interrupt_q = queue.Queue()
speech_q = queue.Queue()
stop_event = threading.Event()

MODE = "WAKE" 
SPEAKING = False 
conversation_history = [] 
MAX_HISTORY = 40 

# ================= UTIL ===================
def internet_available():
    try:
        requests.get("https://www.google.com/generate_204", timeout=2)
        return True
    except:
        return False

# ================= LOCAL SKILLS =================
def run_skill(query):
    q = query.lower()
    if "tell me a joke" in q:
        return "Why do programmers prefer dark mode? Because light attracts bugs."
    if "are you smart" in q:
        return "I am learning every day."
    if "repeat" in q:
        return "Sure. Please tell me what to repeat."
    if "help me" in q:
        return "You can ask me questions, system status, or say stop to interrupt."
    # --- GENERIC VOLUME CONTROL ---
    if "volume" in q or "sound" in q:
        level = parse_volume_percentage(q)
        if level is not None:
            set_volume_level(level)
            return f"Volume set to {int(level * 100)} percent."
    if "mute" in q:
        set_volume_mute(True)
        return "Volume muted."
    if "unmute" in q or "sound on" in q:
        set_volume_mute(False)
        return "Volume unmuted."
    if q in ["hello", "hi", "hey"]:
        return "Hello! How can I help you?"
    if "good morning" in q:
        return "Good morning! I hope you have a great day."
    if "good night" in q:
        return "Good night. Sleep well!"
    if q in ["yes", "yeah", "yep"]:
        return "Okay."
    if q in ["okay", "nope"]:
        return "Alright."
    if "time" in q:
        return f"The time is {time.strftime('%I:%M %p')}."
    if "date" in q or "day" in q or "today" in q or "day is it" in q or "what day is it" in q or "what is the date" in q or "current date" in q:
        return f"Today is {time.strftime('%A, %B %d')}."
    if "how are you" in q:
        return "I am doing great, thank you for asking!"
    if "what is your name" in q:
        return "I am Siri, your voice assistant."
    if "your founder" in q or "who created you" in q or "who made you" in q or "who developed you" in q or "who built you" in q or "who is your creator" in q or "who invented you" in q or"who is your father" in q:
        return "I was created by Gauresh, A MSC Electronics student."
    if "battery" in q:
        battery = psutil.sensors_battery()
        if battery:
            percent = int(battery.percent)
            status = "charging" if battery.power_plugged else ""
            return f"Battery is {percent} percent {status}".strip() + "."
        return "Battery information is not available."
    if "cpu" in q:
        return f"CPU usage is {psutil.cpu_percent(interval=0.5)} percent."
    if "ram" in q or "memory" in q:
        return f"RAM usage is {int(psutil.virtual_memory().percent)} percent."
    if "shutdown" in q:
        os.system("shutdown /s /t 5")
        return "Shutting down in 5 seconds."
    if "restart" in q:
        os.system("shutdown /r /t 5")
        return "Restarting in 5 seconds."
    
    return None

# ================= AI MODELS ==============
llm = Llama(
    model_path=PHI2_MODEL_PATH,
    n_ctx=2048,
    n_threads=6,
    n_batch=256,
    verbose=False
)

SYSTEM_PROMPT = "You are Siri, a helpful and friendly voice assistant. Give short, clear answers. Be conversational but concise."

def ask_offline(prompt):
    memory = ""
    for m in conversation_history[-10:]:
         memory += f"{m['role']}: {m['content']}\n"
    full = f"<|system|>{memory}<|user|>{prompt}<|assistant|>"
    out = ""
    for chunk in llm(full, max_tokens=300, stream=True, temperature=0.4, stop=["<|user|>"]):
        out += chunk["choices"][0]["text"]
    return out.strip()

def ask_online_stream(prompt, history=None):
    if not GROQ_API_KEY:
        raise RuntimeError("GROQ_API_KEY not set")
    
    messages = [{"role": "system", "content": SYSTEM_PROMPT}]
    if history:
        messages += history
    messages.append({"role": "user", "content": prompt})

    headers = {"Authorization": f"Bearer {GROQ_API_KEY}", "Content-Type": "application/json"}
    payload = {"model": GROQ_MODEL, "messages": messages, "temperature": 0.4, "max_tokens": 300, "stream": True}
    
    r = requests.post(GROQ_URL, headers=headers, json=payload, timeout=8, stream=True)
    
    full_response = ""
    sentence_buffer = ""
    
    for line in r.iter_lines():
        if not line:
            continue
        decoded_line = line.decode('utf-8').replace('data: ', '')
        if decoded_line == '[DONE]':
            break
        try:
            content = json.loads(decoded_line)['choices'][0]['delta'].get('content', '')
            if content:
                full_response += content
                sentence_buffer += content
                if any(p in sentence_buffer for p in ['.', '!', '?']):
                    speak(sentence_buffer.strip())
                    sentence_buffer = ""
        except:
            continue
                
    if sentence_buffer.strip():
        speak(sentence_buffer.strip())
        
    return full_response.strip()

def ask_brain(prompt, history=None):
    # Check local skills first
    skill_result = run_skill(prompt)
    if skill_result:
        print("🛠️ LOCAL SKILL")
        speak(skill_result)
        return skill_result

    # Try online AI
    if internet_available() and GROQ_API_KEY:
        try:
            print("🌐 ONLINE AI")
            return ask_online_stream(prompt, history)
        except Exception as e:
            print(f"Online error: {e}")
    
    # Fallback to offline
    print("📴 OFFLINE AI")
    response = ask_offline(prompt)
    speak(response)
    return response

# ================= INTERRUPT DETECTION =================
def interrupt_callback(indata, frames, time_info, status):
    interrupt_q.put((indata[:, 0] * 10 * 32767).astype(np.int16).tobytes())

def interrupt_listener_worker():
    global SPEAKING
    recognizer = KaldiRecognizer(Model(VOSK_MODEL_DIR), SAMPLE_RATE)
    interrupt_words = ["stop", "quiet", "shut", "cancel", "silence", "no", "pause", "wait", "hold", "enough", "break", "halt", "cease", "terminate", "end"]
    last_interrupt = 0
    
    print("🎧 Interrupt listener started")
    
    with sd.InputStream(samplerate=SAMPLE_RATE, channels=1, callback=interrupt_callback, blocksize=2000):
        while True:
            if interrupt_q.empty() or not SPEAKING:
                time.sleep(0.001)
                continue
            
            data = interrupt_q.get()
            
            # Debounce
            if time.time() - last_interrupt < 1.0:
                continue
            
            # Check partial results for fast detection
            partial = json.loads(recognizer.PartialResult()).get("partial", "").lower().strip()
            if partial and any(word in partial.split() for word in interrupt_words):
                print(f"🛑 Interrupt: '{partial}'")
                last_interrupt = time.time()
                handle_interrupt()
                recognizer = KaldiRecognizer(Model(VOSK_MODEL_DIR), SAMPLE_RATE)
                continue
            
            # Check full results
            if recognizer.AcceptWaveform(data):
                result = json.loads(recognizer.Result()).get("text", "").lower().strip()
                if result and any(word in result.split() for word in interrupt_words):
                    print(f"🛑 Interrupt: '{result}'")
                    last_interrupt = time.time()
                    handle_interrupt()
                    recognizer = KaldiRecognizer(Model(VOSK_MODEL_DIR), SAMPLE_RATE)

def handle_interrupt():
    global SPEAKING
    
    stop_event.set()
    SPEAKING = False
    
    try:
        sd.stop()
    except:
        pass
    
    # Clear queues
    for q in [speech_q, interrupt_q, audio_q]:
        while not q.empty():
            try:
                q.get_nowait()
            except:
                break
    
    time.sleep(0.2)
    print("✅ Ready...")

# ================= STT ====================
def stt_callback(indata, frames, time_info, status):
    audio_q.put((indata[:, 0] * 10 * 32767).astype(np.int16).tobytes())

vosk_model = Model(VOSK_MODEL_DIR)

def listen_sentence(timeout):
    recognizer = KaldiRecognizer(vosk_model, SAMPLE_RATE)
    audio_q.queue.clear()

    last_voice = time.time()
    start = time.time()

    print("🎤 Listening...")
    with sd.InputStream(samplerate=SAMPLE_RATE, channels=1, callback=stt_callback):
        while time.time() - start < timeout:
            if audio_q.empty():
                time.sleep(0.01)
                continue
                
            data = audio_q.get()

            if recognizer.AcceptWaveform(data):
                text = json.loads(recognizer.Result()).get("text", "").lower()
                if len(text.split()) >= MIN_WORDS:
                    return text

            partial = json.loads(recognizer.PartialResult()).get("partial", "")
            if partial:
                last_voice = time.time()

            if time.time() - last_voice > SILENCE_END:
                final = json.loads(recognizer.FinalResult()).get("text", "").lower()
                if len(final.split()) >= MIN_WORDS:
                    return final
                return ""
    return ""

# ================= TTS ====================
def speech_worker():
    global SPEAKING
    while True:
        text = speech_q.get()
        if text is None:
            break
        
        stop_event.clear()
        SPEAKING = True
        process = None
        
        try:
            process = subprocess.Popen(
                ["piper", "--model", PIPER_MODEL, "--output_raw"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL
            )
            process.stdin.write(text.encode())
            process.stdin.close()
            
            with sd.RawOutputStream(samplerate=22050, blocksize=256, channels=1, dtype='int16') as stream:
                while True:
                    if stop_event.is_set():
                        process.kill()
                        break
                    audio_chunk = process.stdout.read(256)
                    if not audio_chunk:
                        break
                    stream.write(audio_chunk)
            
            if process:
                process.wait(timeout=0.1)
        except:
            pass
        finally:
            if process and process.poll() is None:
                process.kill()
            SPEAKING = False
            speech_q.task_done()

def speak(text):
    if text:
        speech_q.put(text)

# ================= STARTUP ===============
threading.Thread(target=speech_worker, daemon=True).start()
threading.Thread(target=interrupt_listener_worker, daemon=True).start()

porcupine = pvporcupine.create(access_key=ACCESS_KEY, keywords=[WAKE_WORD])
recorder = PvRecorder(device_index=DEVICE_INDEX, frame_length=porcupine.frame_length)
recorder.start()

print("🟢 Assistant running. Say:", WAKE_WORD)

# ================= MAIN LOOP ===============
try:
    while True:
        if MODE == "WAKE":
            pcm = recorder.read()
            if porcupine.process(pcm) >= 0:
                MODE = "ACTIVE"
                print("👂 Wake word detected")
                time.sleep(0.3)
                speak("Yes? Siri here")
                conversation_history.clear()

        elif MODE == "ACTIVE":
            while SPEAKING or not speech_q.empty():
                time.sleep(0.1)
                
            query = listen_sentence(8)
            if not query:
                speak("I did not hear anything")
                MODE = "WAKE"
                continue

            print("🗣️ You:", query)
            response = ask_brain(query, conversation_history)
            
            conversation_history.append({"role": "user", "content": query})
            conversation_history.append({"role": "assistant", "content": response})
            if len(conversation_history) > MAX_HISTORY:
                conversation_history = conversation_history[-MAX_HISTORY:]

            MODE = "FOLLOWUP"
            follow_start = time.time()

        elif MODE == "FOLLOWUP":
            if time.time() - follow_start > FOLLOWUP_TIMEOUT:
                MODE = "WAKE"
                continue
            
            while SPEAKING or not speech_q.empty():
                time.sleep(0.1)
            
            follow = listen_sentence(6)
            if follow:
                print("🗣️ Follow-up:", follow)
                if any(word in follow for word in ["thank you", "bye", "goodbye", "see you", "stop here", "that's all","go to sleep"]):
                    speak("You're welcome. Goodbye!")
                    MODE = "WAKE"
                    continue
                    
                response = ask_brain(follow, conversation_history)
                
                conversation_history.append({"role": "user", "content": follow})
                conversation_history.append({"role": "assistant", "content": response})
                follow_start = time.time()

except KeyboardInterrupt:
    print("\n👋 Shutting down...")
finally:
    recorder.stop()
    recorder.delete()
    porcupine.delete()