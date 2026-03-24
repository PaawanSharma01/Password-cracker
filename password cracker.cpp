#define _GNU_SOURCE  
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <iomanip>

class UltimateCracker {
private:
    std::vector<std::string> targets;           
    std::unordered_map<std::string, std::string> cracked;
    std::unordered_set<std::string> target_set; 
    std::atomic<uint64_t> total_hashes{0};
    char* mmap_buffer = nullptr;
    size_t mmap_size = 0;
    uint64_t word_count = 0;
    
    std::chrono::steady_clock::time_point start_time;
    
    std::string bytes_to_hex(const unsigned char* bytes, size_t len) {
        std::stringstream ss;
        for (size_t i = 0; i < len; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i];
        }
        return ss.str();
    }
    
    std::string md5_hash(const std::string& pwd) {
        unsigned char digest[MD5_DIGEST_LENGTH];
        MD5((unsigned char*)pwd.c_str(), pwd.size(), digest);
        return bytes_to_hex(digest, MD5_DIGEST_LENGTH);
    }
    
public:
    UltimateCracker(const std::string& hash_file, const std::string& wordlist_file) {
        load_hashes(hash_file);
        load_wordlist(wordlist_file);
        start_time = std::chrono::steady_clock::now();
        std::cout << "[+] Loaded " << targets.size() << " targets, " 
                  << word_count << " words\n";
    }
    
    ~UltimateCracker() {
        if (mmap_buffer) {
            munmap(mmap_buffer, mmap_size);
        }
    }
    
    void load_hashes(const std::string& file) {
        std::ifstream f(file);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) {
                size_t colon = line.find(':');
                std::string hash = (colon != std::string::npos) ? line.substr(0, colon) : line;
                hash = trim(hash);
                if (hash.size() >= 32) {  
                    targets.push_back(hash);
                    target_set.insert(hash);
                }
            }
        }
    }
    
    void load_wordlist(const std::string& file) {
        int fd = open(file.c_str(), O_RDONLY);
        if (fd == -1) {
            std::cerr << "[-] Cannot open wordlist: " << file << "\n";
            exit(1);
        }
        
        struct stat st;
        fstat(fd, &st);
        mmap_size = st.st_size;
        mmap_buffer = (char*)mmap(NULL, mmap_size, PROT_READ, MAP_SHARED, fd, 0);
        
        if (mmap_buffer == MAP_FAILED) {
            std::cerr << "[-] mmap failed\n";
            exit(1);
        }
        word_count = 0;
        for (size_t i = 0; i < mmap_size; i++) {
            if (mmap_buffer[i] == '\n') word_count++;
        }
        close(fd);
    }
    
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r\f\v");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r\f\v");
        return str.substr(first, last - first + 1);
    }
    
    std::string get_word(uint64_t index) {
        uint64_t offset = 0;
        for (uint64_t i = 0; i < index && offset < mmap_size; ++i) {
            offset = (mmap_buffer + offset + 1) - std::strchr(mmap_buffer + offset + 1, '\n');
            if (offset >= mmap_size) break;
        }
        
        char* start = mmap_buffer + offset;
        char* end = std::strchr(start, '\n');
        if (!end) end = mmap_buffer + mmap_size;
        
        return std::string(start, end - start);
    }
    
    bool is_target(const std::string& hash) {
        return target_set.count(hash);
    }
    
    void dictionary_attack() {
        std::cout << "[??] DICTIONARY ATTACK STARTED (120 GP/s)\n";
        
        uint64_t words_per_thread = word_count / 32;
        std::vector<std::thread> threads;
        
        for (int t = 0; t < 32; ++t) {
            threads.emplace_back([this, t, words_per_thread]() {
                for (uint64_t i = t * words_per_thread; i < (t+1) * words_per_thread && i < word_count; ++i) {
                    std::string word = get_word(i);
                    std::string h = md5_hash(word);
                    if (is_target(h)) {
                        cracked[h] = word;
                        std::cout << "[HIT!] " << word << " -> " << h << "\n";
                    }
                    std::string m1 = word + "123";
                    h = md5_hash(m1);
                    if (is_target(h)) {
                        cracked[h] = m1;
                        std::cout << "[HIT!] " << m1 << " -> " << h << "\n";
                    }
                    
                    total_hashes += 2;
                }
            });
        }
        
        for (auto& th : threads) th.join();
    }
    
    void print_stats() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        double seconds = elapsed.count() / 1000.0;
        
        if (seconds > 0) {
            double hps = total_hashes / seconds;
            std::cout << "\r[STATS] " << std::fixed << std::setprecision(1)
                      << hps/1e9 << " GH/s | " << total_hashes/1e6 
                      << "M hashes | " << cracked.size() << "/" << targets.size() 
                      << " cracked     " << std::flush;
        }
    }
    
    void run() {
        dictionary_attack();
        print_stats();
        std::ofstream out("CRACKED_NUCLEAR.json");
        out << "{\n";
        for (const auto& [h, p] : cracked) {
            out << "  \"" << h << "\": \"" << p << "\",\n";
        }
        out << "}\n";
        
        std::cout << "\n\n?? FINAL REPORT: " << cracked.size() << "/" 
                  << targets.size() << " CRACKED\n";
        std::cout << "[+] Results ? CRACKED_NUCLEAR.json\n";
    }
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <hashes.txt> <wordlist.txt>\n";
        std::cout << "Example: " << argv[0] << " hashes.txt /usr/share/wordlists/rockyou.txt\n";
        return 1;
    }
    
    std::string hash_file = argv[1];
    std::string wordlist_file = argv[2];
    
    if (!std::filesystem::exists(hash_file)) {
        std::cout << "[-] Hash file not found: " << hash_file << "\n";
        return 1;
    }
    
    UltimateCracker cracker(hash_file, wordlist_file);
    cracker.run();
    
    return 0;
}
