#pragma once
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>

struct OffsetEntry {
    std::string name;
    uintptr_t offset;
    bool found;
    std::string description;
    std::string category;
};

class OutputGenerator {
private:
    std::vector<OffsetEntry> offsets;
    std::string robloxVersion;
    std::string byfronVersion;
    
public:
    void AddOffset(const std::string& name, uintptr_t offset, bool found, 
                   const std::string& desc = "", const std::string& category = "General") {
        offsets.push_back({name, offset, found, desc, category});
    }
    
    void SetRobloxVersion(const std::string& version) { robloxVersion = version; }
    void SetByfronVersion(const std::string& version) { byfronVersion = version; }
    
    void GenerateOutput();
    int GetFoundCount() const;
    int GetTotalCount() const { return offsets.size(); }
    
private:
    void GenerateHeaderFile();
    void GenerateTextFile();
    void GenerateJsonFile();
    std::string GetCurrentTimestamp();
};