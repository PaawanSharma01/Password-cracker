# Password-cracker
UltimateCracker – High-Speed MD5 Password Cracker

UltimateCracker is a high-performance, multithreaded dictionary-based MD5 password cracking tool written in C++. It is designed to efficiently process large wordlists and crack hashed passwords using optimized techniques like memory mapping and parallel execution.

Features
Blazing Fast Performance
Utilizes 32 threads for parallel processing to maximize hash computation speed.

Dictionary Attack Engine
Performs dictionary-based attacks with basic mutation (word + "123").

Memory-Mapped Wordlist (mmap)
Efficiently loads large wordlists into memory for faster access and reduced I/O overhead.

Target Hash Matching
Uses an unordered_set for O(1) lookup of target hashes.

Real-Time Statistics
Displays hash rate (GH/s), total hashes processed, and cracking progress.
JSON Output
Saves cracked passwords into a structured file: CRACKED_NUCLEAR.json.
How It Works
Loads target hashes from a file.
Maps the wordlist into memory using mmap.
Splits workload across 32 threads.
Each thread:
Reads words from the wordlist
Generates MD5 hashes
Applies simple mutations
Compares results with target hashes
Matches are stored and printed instantly.

Usage
g++ -o cracker password_cracker.cpp -lssl -lcrypto -pthread
./cracker <hashes.txt> <wordlist.txt>
Example:
./cracker hashes.txt /usr/share/wordlists/rockyou.txt

Input Format
hashes.txt
5f4dcc3b5aa765d61d8327deb882cf99
098f6bcd4621d373cade4e832627b4f6

Output

Results are saved in:

CRACKED_NUCLEAR.json

Example:

{
  "5f4dcc3b5aa765d61d8327deb882cf99": "password",
  "098f6bcd4621d373cade4e832627b4f6": "test"
}
Disclaimer

This tool is intended for educational and ethical security research purposes only.
Do NOT use it on systems or data without proper authorization.

Notes
Uses OpenSSL’s MD5 implementation
Optimized for large datasets and high-speed cracking
Performance depends on CPU cores and wordlist size
