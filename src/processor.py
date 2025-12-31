import sys
import os
import shutil
import re
import subprocess

# Fix PATH for venv
venv_bin = os.path.dirname(sys.executable)
os.environ["PATH"] = venv_bin + os.pathsep + os.environ["PATH"]

def sanitize_filename(name):
    return re.sub(r'[^\w\s-]', '', name).strip()

def process_song(song_name):
    print(f"[Python] Processing request: {song_name}")

    # SETUP LIBRARY PATHS
    safe_name = sanitize_filename(song_name)
    if not safe_name: safe_name = "Unknown_Song"
        
    library_dir = os.path.join("library", safe_name)

    # We check for 'drums.wav' as a proxy for success
    if os.path.exists(os.path.join(library_dir, "drums.wav")):
        print(f"[Python] Stems already exist in library!")
        print(f"OUTPUT:{safe_name}")
        return

    os.makedirs(library_dir, exist_ok=True)

    # 2. DOWNLOAD
    print(f"[Python] 1. Downloading...")
    temp_download = "temp_download"
    # Cleanup old temp files
    for ext in [".mp3", ".webm", ".m4a"]:
        if os.path.exists(temp_download + ext): os.remove(temp_download + ext)

    cmd_download = [
        "yt-dlp", f"ytsearch1:{song_name}",
        "-x", "--audio-format", "mp3",
        "-o", f"{temp_download}.%(ext)s", "--force-overwrites"
    ]
    subprocess.run(cmd_download, check=True)
    download_file = f"{temp_download}.mp3"

    # 3. SEPARATE
    print(f"[Python] 2. Separating Stems...")
    if os.path.exists("separated_output"): shutil.rmtree("separated_output")

    cmd_separate = ["demucs", "-n", "htdemucs", download_file, "-o", "separated_output"]
    subprocess.run(cmd_separate, check=True)

    source_stem_path = os.path.join("separated_output", "htdemucs", temp_download)

    # 4. MOVE STEMS TO LIBRARY
    print(f"[Python] 3. Saving Stems to: {library_dir}")
    stems = ["vocals.wav", "bass.wav", "drums.wav", "other.wav"]
    
    for stem in stems:
        src = os.path.join(source_stem_path, stem)
        dst = os.path.join(library_dir, stem)
        if os.path.exists(src):
            shutil.move(src, dst)
        else:
            print(f"WARNING: Stem not found: {stem}")

    # 5. CLEANUP
    if os.path.exists(download_file): os.remove(download_file)
    if os.path.exists("separated_output"): shutil.rmtree("separated_output")

    print(f"[Python] Success!")
    print(f"OUTPUT:{safe_name}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        process_song(sys.argv[1])
    else:
        print("Error: No song name provided")