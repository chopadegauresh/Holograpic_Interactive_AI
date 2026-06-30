import os
import requests

from assistant import OFFLINE_SPEED, OFFLINE_SPEED, ONLINE_SPEED

GEMINI_API_KEY = os.getenv("GEMINI_API_KEY")

url = "https://generativelanguage.googleapis.com/v1beta/models"

response = requests.get(
    f"{url}?key={GEMINI_API_KEY}",
    timeout=10
)

print(response.status_code)
print(response.text)



###################################################


# =========================================================
#                ALL-IN-ONE VOICE ASSISTANT
# =========================================================

import os
import time
import json
import queue
import subprocess
import requests
import numpy as np
import sounddevice as sd
import soundfile as sf
import pvporcupine
from pvrecorder import PvRecorder
from vosk import Model, KaldiRecognizer
from llama_cpp import Llama

# ================= CONFIG =================
ACCESS_KEY = "HaAyTSoHm4saCsEHj2lzL6E+ftm+2FjAyjP3zWt1RHxAOAUuJSe5JA=="
WAKE_WORD = "hey siri"

VOSK_MODEL_DIR = "vosk-model-small-en-us-0.15"
PIPER_MODEL = "piper_voices/en_US-libritts_r-medium.onnx"

# Offline model
PHI2_MODEL_PATH = "models/phi2.gguf"

# Online (Groq)
GROQ_API_KEY = os.getenv("GROQ_API_KEY")
GROQ_URL = "https://api.groq.com/openai/v1/chat/completions"
GROQ_MODEL = "llama-3.1-8b-instant"

SAMPLE_RATE = 16000
STT_DEVICE = None
# =========================================

# ================= GLOBAL STATE =================
audio_q = queue.Queue()
assistant_active = False
# ===============================================


# ================== UTILITIES ===================
def internet_available():
    try:
        requests.get("https://www.google.com", timeout=2)
        return True
    except:
        return False
# ===============================================


# ================== OFFLINE AI ==================
llm = Llama(
    model_path=PHI2_MODEL_PATH,
    n_ctx=2048,
    n_threads=6,
    n_batch=256,
    verbose=False
)

SYSTEM_PROMPT = (
    "You are a fast, smart voice assistant. "
    "Give short, clear, accurate answers."
)

def ask_offline_stream(prompt):
    full_prompt = f"<|system|>{SYSTEM_PROMPT}<|user|>{prompt}<|assistant|>"
    for chunk in llm(
        full_prompt,
        max_tokens=200,
        stream=True,
        stop=["<|user|>"]
    ):
        token = chunk["choices"][0]["text"]
        if token.strip():
            yield token
# ===============================================


# ================== ONLINE AI ===================
def ask_online(prompt: str) -> str:
    if not GROQ_API_KEY:
        raise RuntimeError("GROQ_API_KEY not set")

    headers = {
        "Authorization": f"Bearer {GROQ_API_KEY}",
        "Content-Type": "application/json"
    }

    payload = {
        "model": GROQ_MODEL,
        "messages": [
            {
                "role": "system",
                "content": "You are a fast, helpful voice assistant. Answer clearly and concisely."
            },
            {
                "role": "user",
                "content": prompt
            }
        ],
        "temperature": 0.4,
        "max_tokens": 200
    }

    response = requests.post(
        GROQ_URL,
        headers=headers,
        json=payload,
        timeout=10
    )

    if response.status_code != 200:
        raise RuntimeError(response.text)

    return response.json()["choices"][0]["message"]["content"].strip()
# ===============================================


# ================== ROUTER ======================
def ask_brain(prompt: str):
    if internet_available():
        try:
            global CURRENT_SPEED
            CURRENT_SPEED = ONLINE_SPEED
            print("🌐 USING ONLINE AI (GROQ)")
            return ask_online(prompt)
        except Exception as e:
            print("⚠️ ONLINE FAILED → OFFLINE", e)
    global CURRENT_SPEED
    CURRENT_SPEED = OFFLINE_SPEED
    print("📴 USING OFFLINE AI")
    return "".join(ask_offline_stream(prompt))
# ===============================================


# ================== STT =========================
def stt_callback(indata, frames, time_info, status):
    pcm16 = (indata[:, 0] * 32767).astype(np.int16).tobytes()
    audio_q.put(pcm16)

vosk_model = Model(VOSK_MODEL_DIR)

def listen_for_command(max_wait=8):
    recognizer = KaldiRecognizer(vosk_model, SAMPLE_RATE)
    audio_q.queue.clear()

    last_voice = time.time()
    start = time.time()

    with sd.InputStream(
        samplerate=SAMPLE_RATE,
        channels=1,
        device=STT_DEVICE,
        callback=stt_callback
    ):
        while time.time() - start < max_wait:
            if not audio_q.empty():
                data = audio_q.get()

                if recognizer.AcceptWaveform(data):
                    text = json.loads(recognizer.Result()).get("text", "")
                    if text.strip():
                        return text.lower()

                partial = json.loads(recognizer.PartialResult()).get("partial", "")
                if partial:
                    last_voice = time.time()

                if time.time() - last_voice > 2.0:
                    final = json.loads(recognizer.FinalResult()).get("text", "")
                    return final.lower()

    return ""
# ===============================================


# ================== TTS =========================
def speak(text):
    if not text.strip():
        return

    audio_q.queue.clear()

    subprocess.run(
        [
            "piper",
            "--model", PIPER_MODEL,
            "--length_scale", "1.2",
            "--noise_scale", "0.5",
            "--noise_w", "0.7",
            "--output_file", "reply.wav"
        ],
        input=text.encode("utf-8"),
        check=False
    )

    data, sr = sf.read("reply.wav", dtype="float32")
    sd.play(data, sr)
    sd.wait()
# ===============================================


# ================== WAKE WORD ===================
porcupine = pvporcupine.create(
    access_key=ACCESS_KEY,
    keywords=[WAKE_WORD]
)

recorder = PvRecorder(
    device_index=-1,
    frame_length=porcupine.frame_length
)

print("🟢 Assistant running. Say:", WAKE_WORD)
recorder.start()
# ===============================================


# ================== MAIN LOOP ===================
try:
    while True:
        pcm = recorder.read()

        if not assistant_active and porcupine.process(pcm) >= 0:
            assistant_active = True
            print("👂 Wake word detected")

            speak("Yes, I am listening")

            query = listen_for_command()
            if not query:
                speak("I did not hear anything")
                assistant_active = False
                continue

            print("🗣️ You said:", query)

            response = ask_brain(query)
            speak(response)

            assistant_active = False

except KeyboardInterrupt:
    pass
finally:
    recorder.stop()
    recorder.delete()
    porcupine.delete()
# ===============================================

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#above one is only single talk code 

#and below one is continues talk after wake word detected and for 10 __spec__

# =========================================================
#                ALL-IN-ONE VOICE ASSISTANT
# =========================================================

import os, time, json, queue, subprocess, requests
import numpy as np
import sounddevice as sd
import soundfile as sf
import pvporcupine
from pvrecorder import PvRecorder
from vosk import Model, KaldiRecognizer
from llama_cpp import Llama

# ================= CONFIG =================
ACCESS_KEY = "HaAyTSoHm4saCsEHj2lzL6E+ftm+2FjAyjP3zWt1RHxAOAUuJSe5JA=="
WAKE_WORD = "hey siri"

VOSK_MODEL_DIR = "vosk-model-small-en-us-0.15"
PIPER_MODEL = "piper_voices/en_US-libritts_r-medium.onnx"
PHI2_MODEL_PATH = "models/phi2.gguf"

FOLLOWUP_TIMEOUT = 10      # seconds
MIN_WORDS = 2              # ignore junk like "my"

GROQ_API_KEY = os.getenv("GROQ_API_KEY")
GROQ_URL = "https://api.groq.com/openai/v1/chat/completions"
GROQ_MODEL = "llama-3.1-8b-instant"

SAMPLE_RATE = 16000
# =========================================

# ================= STATE ==================
audio_q = queue.Queue()
MODE = "WAKE"   # WAKE | ACTIVE | FOLLOWUP
# =========================================

# ================= UTIL ===================
def internet_available():
    try:
        requests.get("https://www.google.com", timeout=2)
        return True
    except:
        return False
# =========================================

# ================= OFFLINE AI ==============
llm = Llama(
    model_path=PHI2_MODEL_PATH,
    n_ctx=2048,
    n_threads=6,
    n_batch=256,
    verbose=False
)

SYSTEM_PROMPT = (
    "You are a fast, smart voice assistant. "
    "Give short, clear, accurate answers."
)

def ask_offline(prompt):
    full = f"<|system|>{SYSTEM_PROMPT}<|user|>{prompt}<|assistant|>"
    out = ""
    for chunk in llm(full, max_tokens=200, stream=True):
        out += chunk["choices"][0]["text"]
    return out.strip()
# =========================================

# ================= ONLINE AI ===============
def ask_online(prompt):
    headers = {
        "Authorization": f"Bearer {GROQ_API_KEY}",
        "Content-Type": "application/json"
    }
    payload = {
        "model": GROQ_MODEL,
        "messages": [
            {"role": "system", "content": "You are a fast voice assistant."},
            {"role": "user", "content": prompt}
        ],
        "temperature": 0.4,
        "max_tokens": 200
    }
    r = requests.post(GROQ_URL, headers=headers, json=payload, timeout=8)
    return r.json()["choices"][0]["message"]["content"].strip()
# =========================================

# ================= ROUTER =================
def ask_brain(prompt):
    if internet_available() and GROQ_API_KEY:
        try:
            print("🌐 USING ONLINE AI (GROQ)")
            return ask_online(prompt)
        except:
            pass
    print("📴 USING OFFLINE AI")
    return ask_offline(prompt)
# =========================================

# ================= STT ====================
def stt_callback(indata, frames, time_info, status):
    audio_q.put((indata[:, 0] * 32767).astype(np.int16).tobytes())

vosk_model = Model(VOSK_MODEL_DIR)

def listen_command(timeout):
    recognizer = KaldiRecognizer(vosk_model, SAMPLE_RATE)
    audio_q.queue.clear()

    last_voice = time.time()
    start = time.time()

    with sd.InputStream(samplerate=SAMPLE_RATE, channels=1, callback=stt_callback):
        while time.time() - start < timeout:
            if not audio_q.empty():
                data = audio_q.get()
                if recognizer.AcceptWaveform(data):
                    text = json.loads(recognizer.Result()).get("text", "").lower()
                    if len(text.split()) >= MIN_WORDS:
                        return text
                else:
                    partial = json.loads(recognizer.PartialResult()).get("partial", "")
                    if partial:
                        last_voice = time.time()
                if time.time() - last_voice > 1.2:
                    final = json.loads(recognizer.FinalResult()).get("text", "").lower()
                    if len(final.split()) >= MIN_WORDS:
                        return final
    return ""
# =========================================

# ================= TTS ====================
def speak(text):
    if not text:
        return
    subprocess.run(
        ["piper", "--model", PIPER_MODEL, "--output_file", "reply.wav"],
        input=text.encode(),
        check=False
    )
    data, sr = sf.read("reply.wav", dtype="float32")
    sd.play(data, sr)
    sd.wait()
# =========================================

# ================= WAKE WORD ===============
porcupine = pvporcupine.create(access_key=ACCESS_KEY, keywords=[WAKE_WORD])
recorder = PvRecorder(device_index=-1, frame_length=porcupine.frame_length)
recorder.start()
print("🟢 Assistant running. Say:", WAKE_WORD)
# =========================================

# ================= MAIN LOOP ===============
try:
    while True:
        if MODE == "WAKE":
            pcm = recorder.read()
            if porcupine.process(pcm) >= 0:
                MODE = "ACTIVE"
                print("👂 Wake word detected")
                speak("Yes, I am listening")

        if MODE == "ACTIVE":
            query = listen_command(8)
            if not query:
                speak("I did not hear anything")
                MODE = "WAKE"
                continue

            print("🗣️ You said:", query)
            speak(ask_brain(query))
            MODE = "FOLLOWUP"
            follow_start = time.time()

        if MODE == "FOLLOWUP":
            if time.time() - follow_start > FOLLOWUP_TIMEOUT:
                MODE = "WAKE"
                continue

            follow = listen_command(3)
            if follow:
                print("🗣️ Follow-up:", follow)
                speak(ask_brain(follow))
                follow_start = time.time()

except KeyboardInterrupt:
    pass
finally:
    recorder.stop()
    recorder.delete()
    porcupine.delete()
# =========================================
