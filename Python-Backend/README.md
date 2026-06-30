# Python-Backend

This folder contains the backend assistant for the Holographic Interactive AI project.

## Setup

1. Open PowerShell in `Python-Backend`:

```powershell
cd "D:\***\Holographic_Interactive_AI\Python-Backend"
```

2. Create the local virtual environment (if not already created):

```powershell
python -m venv .venv
```

3. Activate the virtual environment:

```powershell
.\.venv\Scripts\Activate.ps1
```

4. Upgrade packaging tools:

```powershell
python -m pip install --upgrade pip setuptools wheel
```

5. Install required dependencies:

```powershell
python -m pip install -r requirements.txt
```

## Run

With the virtual environment active:

```powershell
python assistant.py
```

Or without activation:

```powershell
.\.venv\Scripts\python.exe assistant.py
```

## Notes

- Use the `Python-Backend\.venv` environment for this backend.
- If you have an existing `.venv` in a different folder, make sure the backend is using the correct one.
- The current backend requirements file is `requirements.txt`.
