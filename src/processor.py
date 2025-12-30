import sys
import os
import subprocess

# Fix PATH for venv
venv_bin = os.path.dirname(sys.executable)
os.environ["PATH"] = venv_bin + os.pathsep + os.environ["PATH"]

def process_song(song_name):
    base_filename = "downloaded_song"  # Use this everywhere
    download_file = f"{base_filename}.mp3"
    
    print(f"[Python] 1. Downloading: {song_name}")
    
    if os.path.exists("backing_track.mp3"):
        os.remove("backing_track.mp3")
        
    cmd_download = [
        "yt-dlp",
        f"ytsearch1:{song_name}",
        "-x", "--audio-format", "mp3",
        "-o", f"{base_filename}.%(ext)s",
        "--force-overwrites"
    ]
    subprocess.run(cmd_download, check=True)
    
    print(f"[Python] 2. Separating Stems (Demucs)...")
    cmd_separate = [
        "demucs",
        "-n", "htdemucs", 
        download_file,
        "-o", "separated_output"
    ]
    subprocess.run(cmd_separate, check=True)
    
    # Demucs output path logic
    # It creates a folder with the exact name of the input file (minus extension)
    stem_path = os.path.join("separated_output", "htdemucs", base_filename)
    
    print(f"[Python] 3. Mixing Backing Track from: {stem_path}")
    
    inputs = [
        os.path.join(stem_path, "vocals.wav"),
        os.path.join(stem_path, "bass.wav"),
        os.path.join(stem_path, "drums.wav")
    ]
    
    # DEBUG: Check if files actually exist
    for f in inputs:
        if not os.path.exists(f):
            print(f"ERROR: Missing stem file: {f}")
            if os.path.exists(stem_path):
                print(f"Contents of {stem_path}: {os.listdir(stem_path)}")
            else:
                print(f"Folder {stem_path} does not exist!")
            sys.exit(1)

    cmd_mix = ["ffmpeg", "-y"]
    for i in inputs:
        cmd_mix.extend(["-i", i])
        
    cmd_mix.extend([
        "-filter_complex", f"amix=inputs={len(inputs)}:duration=longest",
        "backing_track.mp3"
    ])
    
    subprocess.run(cmd_mix, check=True)
    
    print(f"[Python] Success! Created 'backing_track.mp3'")
    print("OUTPUT:backing_track.mp3")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        process_song(sys.argv[1])
    else:
        print("Error: No song name provided")