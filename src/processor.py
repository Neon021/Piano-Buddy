import sys
import os
import subprocess

venv_bin = os.path.dirname(sys.executable)
os.environ["PATH"] = venv_bin + os.pathsep + os.environ["PATH"]

def process_song(song_name):
    print(f"[Python] Searching and downloading: {song_name}")
    
    # 1. Download with yt-dlp
    # We download as m4a/bestaudio and save it as 'downloaded_song'
    cmd_download = [
        "yt-dlp",
        f"ytsearch1:{song_name}", # Search and pick first result
        "-x", "--audio-format", "mp3", # Convert to mp3
        "-o", "downloaded_song.%(ext)s", # Output filename
        "--force-overwrites"
    ]
    subprocess.run(cmd_download, check=True)
    
    # 2. Separate with Demucs
    print(f"[Python] Separating stems with Demucs (this may take time)...")
    # -n htdemucs is a faster, high-quality model
    cmd_separate = [
        "demucs",
        "-n", "htdemucs", 
        "downloaded_song.mp3",
        "-o", "separated_output"
    ]
    subprocess.run(cmd_separate, check=True)
    
    print("[Python] Processing complete.")
    # The output will be in: separated_output/htdemucs/downloaded_song/
    # Files: drums.wav, bass.wav, other.wav, vocals.wav

if __name__ == "__main__":
    if len(sys.argv) > 1:
        process_song(sys.argv[1])
    else:
        print("Error: No song name provided")