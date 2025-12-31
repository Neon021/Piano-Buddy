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

    # 1. SETUP LIBRARY PATHS
    safe_name = sanitize_filename(song_name)
    if not safe_name: safe_name = "Unknown_Song"
        
    library_dir = os.path.join("library", safe_name)
    backing_track = os.path.join(library_dir, "backing_track.mp3")

    # If the folder exists and has the backing track, we assume it's done
    if os.path.exists(backing_track):
        print(f"[Python] Song already exists in library!")
        print(f"OUTPUT:{backing_track}")
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

    # Path where Demucs saved the stems
    source_stem_path = os.path.join("separated_output", "htdemucs", temp_download)

    # MOVE STEMS TO LIBRARY
    print(f"[Python] 3. Saving Stems to: {library_dir}")
    stems = ["vocals.wav", "bass.wav", "drums.wav", "other.wav"]
    
    for stem in stems:
        src = os.path.join(source_stem_path, stem)
        dst = os.path.join(library_dir, stem)
        if os.path.exists(src):
            shutil.move(src, dst)
        else:
            print(f"WARNING: Stem not found: {stem}")

    # GENERATE BACKING TRACK
    print(f"[Python] 4. Mixing Backing Track...")
    mix_inputs = [
        os.path.join(library_dir, "vocals.wav"),
        os.path.join(library_dir, "bass.wav"),
        os.path.join(library_dir, "drums.wav"),
        os.path.join(library_dir, "other.wav")
    ]
    
    cmd_mix = ["ffmpeg", "-y"]
    for i in mix_inputs:
        cmd_mix.extend(["-i", i])
        
    cmd_mix.extend([
        "-filter_complex", f"amix=inputs={len(mix_inputs)}:duration=longest",
        backing_track
    ])
    subprocess.run(cmd_mix, check=True)

    # CLEANUP
    if os.path.exists(download_file): os.remove(download_file)
    if os.path.exists("separated_output"): shutil.rmtree("separated_output")

    print(f"[Python] Success!")
    print(f"OUTPUT:{backing_track}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        process_song(sys.argv[1])
    else:
        print("Error: No song name provided")