# Copilot Instructions — AI Voice Assistant

## Architecture
**Single-file monolithic design** (`assistant.py`): wake word → STT → local skills → AI brain → TTS
- **Wake Word**: PvPorcupine detects "hey siri" (configurable) via `WAKE_WORD` constant
- **STT**: Vosk (offline, streaming) returns recognized text when silence detected after speech
- **Brain**: Three-tier fallback: (1) local skills (time, battery, GPIO), (2) Groq streaming if online, (3) Phi-2 local LLM
- **TTS**: Piper subprocess feeds audio to `sounddevice.RawOutputStream`, queueing prevents overlaps
- **State Machine**: `WAKE` (listening for wake word) → `ACTIVE` (listen 8s) → `FOLLOWUP` (listen 6s, 15s timeout)

## Data Flows
- **Audio Capture**: PvRecorder/sounddevice callback → `audio_q` (numpy int16) → Vosk recognizer
- **Speech Output**: `ask_brain()` → `speak(text)` → `speech_q` → background thread → Piper → playback
- **Interrupt Logic**: If user says "stop/quiet/shut up" during `SPEAKING`, `stop_event` halts Piper subprocess
- **Conversation Memory**: `conversation_history` (max 20 entries) + `memory.json` (max 12 turns) → context for Groq/Phi-2

## Key Patterns
- **Config Constants** (ALL_CAPS block at top): `WAKE_WORD`, `PIPER_MODEL`, `PHI2_MODEL_PATH`, `GROQ_API_KEY`, `SAMPLE_RATE=16000`, `DEVICE_INDEX=-1`
- **Local Skills First**: `run_skill(query)` checks before AI to avoid latency (time, date, battery, CPU, RAM, shutdown, GPIO)
- **Streaming Responses**: Online (`ask_online_stream`) outputs by sentence; offline (`ask_offline`) yields tokens
- **Queue-Based Threading**: `speech_q` + `speech_worker()` daemon thread ensures sequential TTS playback
- **Silence Detection**: VAD logic compares `time.time() - last_voice > SILENCE_END` (2.0s); interrupts on keyword detection

## Conventions & Gotchas
- **Don't edit active loops** without understanding `SPEAKING` flag and `speech_q.empty()` waits (prevent mic interference)
- **Audio data format**: Must be int16 bytes for Vosk; callback multiplies input by 10×32767 to normalize
- **Memory persistence**: `conversation_history` (in-RAM) vs `conversation_memory` (on-disk); both append user→assistant pairs
- **Error handling**: Minimal logging; exceptions caught inline with `pass` (check stderr for Piper/Groq failures)
- **Windows-specific**: Audio device indices, Piper executable availability, path separators in subprocess calls

## Run & Debug
```powershell
& venv\Scripts\Activate.ps1; python assistant.py
```
- **Audio devices**: `python -c "import sounddevice as sd; print(sd.query_devices())"`; adjust `DEVICE_INDEX` if needed
- **Phi-2 OOM**: Reduce `n_ctx=2048` or `n_threads=6` in Llama constructor (~30s load time)
- **No Vosk output**: Check mic volume, `SAMPLE_RATE`, and verify `audio_q` receives data in callback
- **Overlapping TTS**: Ensure `stop_event.clear()` before Piper and `speech_q.task_done()` after playback

## Quick Edits
- **Wake word**: Update `WAKE_WORD` + `pvporcupine.create(..., keywords=[...])` call (line ~413)
- **Recorder device**: Change `DEVICE_INDEX` (default `-1` = system default)
- **Add local skill**: Add `if "keyword" in q:` branch in `run_skill()` function
- **LLM context**: Adjust `MAX_HISTORY=20` or `MAX_MEMORY_TURNS=12` for longer conversation context