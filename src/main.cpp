#include <windows.h>
#include <iostream>
#include <format>
#include <chrono>
#include <filesystem>
#include <thread>
#include <cmath>
#include "memory/mem.hpp"
#include "../include/offsets.hpp"
#include "utils/OutputGenerator.hpp"

BYTE* base_address = 0;
OutputGenerator outputGen;
uintptr_t g_datamodel = 0;
uintptr_t g_nameOffset = 0;
uintptr_t g_childrenOffset = 0;
uintptr_t g_players = 0;
uintptr_t g_workspace = 0;
uintptr_t g_localPlayer = 0;

// FORWARD DECLARATIONS - so C++ doesnt get confused
void DumpPlayerSpecificOffsets(uintptr_t player);
void DumpCharacterOffsets(uintptr_t character);
void DumpPartOffsets(uintptr_t part, bool isRootPart = false);
void DumpCameraOffsets(uintptr_t camera);

std::string GetRobloxVersion() {
    return "version-fresh-af";
}

inline void attach() {
    DWORD pid;
    HWND targetWindow = FindWindowA(nullptr, "Roblox");
    if (!targetWindow) {
        std::cout << "[!] roblox window ain't there, fire it up first\n";
        std::cin.get();
        return;
    }
    
    GetWindowThreadProcessId(targetWindow, &pid);
    g_procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    if (!g_procHandle) {
        std::cout << "[!] couldn't hook into roblox process, run as admin maybe?\n";
        std::cin.get();
        return;
    }

    BYTE* base_add2 = memory->LocateModuleBase(pid, "RobloxPlayerBeta.exe");
    if (!base_add2) {
        CloseHandle(g_procHandle);
        std::cout << "[!] couldn't find roblox base, that's sus\n";
        std::cin.get();
        return;
    }
    
    std::cout << std::format("[+] found roblox chillin at: 0x{:X}\n", reinterpret_cast<uintptr_t>(base_add2));
    base_address = base_add2;
}

uintptr_t getdatamodel() {
    uintptr_t fakeDataModel = memory->read<uintptr_t>(reinterpret_cast<uintptr_t>(base_address) + offsets::FakeDataModelPointer);
    return fakeDataModel + offsets::FakeDataModelToRealDatamodel;
}

uintptr_t FindChildByName(uintptr_t parent, const std::string& targetName) {
    if (!parent || !g_childrenOffset) return 0;
    
    try {
        uintptr_t childrenStart = memory->read<uintptr_t>(parent + g_childrenOffset);
        uintptr_t children = memory->read<uintptr_t>(childrenStart);
        
        for (int i = 0; i < 200; i++) {
            uintptr_t child = memory->read<uintptr_t>(children + i * 0x10);
            if (child == 0) break;
            
            std::string name = memory->readstring(memory->read<uintptr_t>(child + g_nameOffset));
            if (name == targetName) {
                return child;
            }
        }
    } catch (...) {}
    return 0;
}

uintptr_t FindHumanoid(uintptr_t character) {
    if (!character) return 0;
    
    try {
        uintptr_t childrenStart = memory->read<uintptr_t>(character + g_childrenOffset);
        uintptr_t children = memory->read<uintptr_t>(childrenStart);
        
        for (int i = 0; i < 50; i++) {
            uintptr_t child = memory->read<uintptr_t>(children + i * 0x10);
            if (child == 0) break;
            
            std::string name = memory->readstring(memory->read<uintptr_t>(child + g_nameOffset));
            if (name == "Humanoid") {
                return child;
            }
        }
    } catch (...) {}
    return 0;
}

uintptr_t FindCharacter(uintptr_t player) {
    if (!player) return 0;
    
    try {
        uintptr_t childrenStart = memory->read<uintptr_t>(player + g_childrenOffset);
        uintptr_t children = memory->read<uintptr_t>(childrenStart);
        
        for (int i = 0; i < 50; i++) {
            uintptr_t child = memory->read<uintptr_t>(children + i * 0x10);
            if (child == 0) break;
            
            std::string name = memory->readstring(memory->read<uintptr_t>(child + g_nameOffset));
            std::string className = memory->readstring(memory->read<uintptr_t>(memory->read<uintptr_t>(child + 0x18) + 0x8));
            if (className == "Model" && name.length() > 2 && name != "Workspace") {
                return child;
            }
        }
    } catch (...) {}
    return 0;
}

void DumpBasicOffsets() {
    std::cout << "[+] bout to scan the basics real quick...\n";
    
    // Name offset - EXPANDED RANGE
    std::cout << "[~] hunting for name offset...\n";
    for (uintptr_t offset = 0x40; offset < 0x200; offset += 8) {
        try {
            std::string name = memory->readstring(memory->read<uintptr_t>(g_datamodel + offset));
            if (name == "Ugc" || name == "LuaApp" || name == "DataModel" || name == "Game") {
                g_nameOffset = offset;
                outputGen.AddOffset("Name", offset, true, "instance name string ptr", "Basic");
                std::cout << std::format("[+] name offset locked in at: 0x{:x}\n", offset);
                break;
            }
        } catch (...) {}
    }
    
    if (!g_nameOffset) {
        std::cout << "[-] name offset ain't showing up, that's weird\n";
        outputGen.AddOffset("Name", 0, false, "instance name string ptr", "Basic");
    }
    
    // PlaceId offset - EXPANDED RANGE  
    std::cout << "[~] looking for placeid...\n";
    for (uintptr_t offset = 0x180; offset < 0x250; offset += 8) {
        try {
            uintptr_t placeid = memory->read<uintptr_t>(g_datamodel + offset);
            if (placeid == offsets::PLACE_ID) {
                outputGen.AddOffset("PlaceId", offset, true, "current place id", "Basic");
                std::cout << std::format("[+] placeid found at: 0x{:x}\n", offset);
                break;
            }
        } catch (...) {}
    }
    
    // GameId offset 
    std::cout << "[~] searching for gameid...\n";
    for (uintptr_t offset = 0x180; offset < 0x250; offset += 8) {
        try {
            uintptr_t gameid = memory->read<uintptr_t>(g_datamodel + offset);
            if (gameid != 0 && gameid != offsets::PLACE_ID && gameid > 1000) {
                outputGen.AddOffset("GameId", offset, true, "current game id", "Basic");
                std::cout << std::format("[+] gameid spotted at: 0x{:x}\n", offset);
                break;
            }
        } catch (...) {}
    }
    
    // Children offset - CRUCIAL FOR EVERYTHING
    std::cout << "[~] children offset is key, let's find it...\n";
    for (uintptr_t offset = 0x40; offset < 0x100; offset += 8) {
        try {
            uintptr_t start = memory->read<uintptr_t>(g_datamodel + offset);
            uintptr_t possible_children = memory->read<uintptr_t>(start);

            for (int i = 0; i < 100; i++) {
                if (!g_nameOffset) continue;
                
                std::string name = memory->readstring(memory->read<uintptr_t>(memory->read<uintptr_t>(possible_children + i * 0x10) + g_nameOffset));
                if (name == "Players" || name == "Workspace" || name == "GuiService" || name == "SoundService") {
                    g_childrenOffset = offset;
                    outputGen.AddOffset("Children", offset, true, "children list start", "Basic");
                    outputGen.AddOffset("ChildrenEnd", 0x8, true, "children list end offset", "Basic");
                    std::cout << std::format("[+] children offset locked at: 0x{:x}\n", offset);
                    goto found_children;
                }
            }
        } catch (...) {}
    }
    found_children:;
    
    if (!g_childrenOffset) {
        std::cout << "[-] children offset missing, gonna be hard to find services\n";
        outputGen.AddOffset("Children", 0, false, "children list start", "Basic");
    }
    
    // Parent offset
    for (uintptr_t offset = 0x40; offset < 0x80; offset += 8) {
        try {
            uintptr_t parent = memory->read<uintptr_t>(g_datamodel + offset);
            if (parent == 0) {
                outputGen.AddOffset("Parent", offset, true, "parent instance ptr", "Basic");
                break;
            }
        } catch (...) {}
    }
    
    // ClassDescriptor offset
    for (uintptr_t offset = 0x10; offset < 0x40; offset += 8) {
        try {
            std::string classname = memory->readstring(memory->read<uintptr_t>(memory->read<uintptr_t>(g_datamodel + offset) + 0x8));
            if (classname == "DataModel") {
                outputGen.AddOffset("ClassDescriptor", offset, true, "class descriptor ptr", "Basic");
                outputGen.AddOffset("ClassDescriptorToClassName", 0x8, true, "descriptor to classname", "Basic");
                break;
            }
        } catch (...) {}
    }
    
    // GameLoaded offset
    for (uintptr_t offset = 0x680; offset < 0x750; offset += 8) {
        try {
            int64_t loaded = memory->read<int64_t>(g_datamodel + offset);
            if (loaded == 31) {
                outputGen.AddOffset("GameLoaded", offset, true, "game loaded state", "Basic");
                break;
            }
        } catch (...) {}
    }
}

// TELEPORT/NOCLIP ESSENTIALS
void DumpPartOffsets(uintptr_t part, bool isRootPart) {
    std::string category = isRootPart ? "RootPart" : "Part";
    std::cout << std::format("[~] {} analysis for position hacks...\n", category);
    
    // Position/CFrame - TELEPORT HACK GOLDMINE
    for (uintptr_t offset = 0x140; offset < 0x180; offset += 4) {
        try {
            float x = memory->read<float>(part + offset);
            float y = memory->read<float>(part + offset + 4);
            float z = memory->read<float>(part + offset + 8);
            if (std::abs(x) < 10000.0f && std::abs(y) < 10000.0f && std::abs(z) < 10000.0f && (x != 0 || y != 0 || z != 0)) {
                std::string offsetName = isRootPart ? "RootPartPosition" : "Position";
                outputGen.AddOffset(offsetName, offset, true, "part position (teleport)", category);
                std::cout << std::format("[+] {} position at: 0x{:x}\n", category, offset);
                break;
            }
        } catch (...) {}
    }
    
    // CFrame rotation
    for (uintptr_t offset = 0x120; offset < 0x160; offset += 4) {
        try {
            float rotX = memory->read<float>(part + offset);
            if (rotX >= -1.0f && rotX <= 1.0f && rotX != 0.0f) {
                std::string offsetName = isRootPart ? "RootPartRotation" : "Rotation";
                outputGen.AddOffset(offsetName, offset, true, "part rotation matrix", category);
                break;
            }
        } catch (...) {}
    }
    
    // Velocity - SPEED/FLY HACKS
    for (uintptr_t offset = 0x150; offset < 0x190; offset += 4) {
        try {
            float velX = memory->read<float>(part + offset);
            float velY = memory->read<float>(part + offset + 4);
            float velZ = memory->read<float>(part + offset + 8);
            if (std::abs(velX) < 1000.0f && std::abs(velY) < 1000.0f && std::abs(velZ) < 1000.0f) {
                std::string offsetName = isRootPart ? "RootPartVelocity" : "Velocity";
                outputGen.AddOffset(offsetName, offset, true, "part velocity (speed hack)", category);
                std::cout << std::format("[+] {} velocity at: 0x{:x}\n", category, offset);
                break;
            }
        } catch (...) {}
    }
    
    if (!isRootPart) {
        // Size
        for (uintptr_t offset = 0x240; offset < 0x280; offset += 4) {
            try {
                float sizeX = memory->read<float>(part + offset);
                float sizeY = memory->read<float>(part + offset + 4);
                float sizeZ = memory->read<float>(part + offset + 8);
                if (sizeX > 0.0f && sizeY > 0.0f && sizeZ > 0.0f && sizeX < 1000.0f) {
                    outputGen.AddOffset("PartSize", offset, true, "part size", "Part");
                    break;
                }
            } catch (...) {}
        }
        
        // CanCollide - NOCLIP HACK
        for (uintptr_t offset = 0x2F0; offset < 0x320; offset += 1) {
            try {
                uint8_t flags = memory->read<uint8_t>(part + offset);
                if (flags != 0) {
                    outputGen.AddOffset("CanCollide", offset, true, "part collision flags (noclip)", "Part");
                    outputGen.AddOffset("CanCollideMask", 0x8, true, "cancollide bit mask", "Part");
                    outputGen.AddOffset("Anchored", offset, true, "part anchored flags", "Part");
                    outputGen.AddOffset("AnchoredMask", 0x2, true, "anchored bit mask", "Part");
                    outputGen.AddOffset("CanTouch", offset, true, "part touch flags", "Part");
                    outputGen.AddOffset("CanTouchMask", 0x10, true, "cantouch bit mask", "Part");
                    std::cout << std::format("[+] collision flags at: 0x{:x} (noclip ready)\n", offset);
                    break;
                }
            } catch (...) {}
        }
        
        // Transparency - VISUAL HACKS
        for (uintptr_t offset = 0xF0; offset < 0x120; offset += 4) {
            try {
                float transparency = memory->read<float>(part + offset);
                if (transparency >= 0.0f && transparency <= 1.0f) {
                    outputGen.AddOffset("Transparency", offset, true, "part transparency", "Part");
                    break;
                }
            } catch (...) {}
        }
    }
}

// AIMBOT GOLDMINE
void DumpCameraOffsets(uintptr_t camera) {
    std::cout << "[~] camera offsets for aimbot incoming...\n";
    
    // CFrame/Position - AIMBOT POSITION
    for (uintptr_t offset = 0x110; offset < 0x150; offset += 4) {
        try {
            float x = memory->read<float>(camera + offset);
            float y = memory->read<float>(camera + offset + 4);
            float z = memory->read<float>(camera + offset + 8);
            if (std::abs(x) < 10000.0f && std::abs(y) < 10000.0f && std::abs(z) < 10000.0f && (x != 0 || y != 0 || z != 0)) {
                outputGen.AddOffset("CameraPos", offset, true, "camera position (aimbot)", "Camera");
                std::cout << std::format("[+] camera pos at: 0x{:x}\n", offset);
                break;
            }
        } catch (...) {}
    }
    
    // CFrame rotation matrix - AIMBOT ROTATION
    for (uintptr_t offset = 0xF0; offset < 0x130; offset += 4) {
        try {
            float rotX = memory->read<float>(camera + offset);
            if (rotX >= -1.0f && rotX <= 1.0f && rotX != 0.0f) {
                outputGen.AddOffset("CameraRotation", offset, true, "camera rotation matrix (aimbot)", "Camera");
                std::cout << std::format("[+] camera rotation at: 0x{:x}\n", offset);
                break;
            }
        } catch (...) {}
    }
    
    // FOV - FOV CHANGER
    for (uintptr_t offset = 0x160; offset < 0x180; offset += 4) {
        try {
            float fov = memory->read<float>(camera + offset);
            if (fov > 1.0f && fov < 120.0f) {
                outputGen.AddOffset("FOV", offset, true, "camera fov (fov changer)", "Camera");
                std::cout << std::format("[+] fov at: 0x{:x} (current: {})\n", offset, fov);
                break;
            }
        } catch (...) {}
    }
    
    // CameraType
    for (uintptr_t offset = 0x160; offset < 0x170; offset += 4) {
        try {
            int cameraType = memory->read<int>(camera + offset);
            if (cameraType >= 0 && cameraType <= 6) {
                outputGen.AddOffset("CameraType", offset, true, "camera type enum", "Camera");
                break;
            }
        } catch (...) {}
    }
    
    // CameraSubject - what camera is following
    for (uintptr_t offset = 0xE0; offset < 0x100; offset += 8) {
        try {
            uintptr_t subject = memory->read<uintptr_t>(camera + offset);
            if (subject != 0) {
                outputGen.AddOffset("CameraSubject", offset, true, "camera subject", "Camera");
                break;
            }
        } catch (...) {}
    }
}

// CHEAT GOLDMINE: Character and Humanoid offsets
void DumpCharacterOffsets(uintptr_t character) {
    std::cout << "[~] character analysis for movement hacks...\n";
    
    uintptr_t humanoid = FindHumanoid(character);
    if (!humanoid) {
        std::cout << "[-] humanoid not found in character\n";
        return;
    }
    
    std::cout << "[+] humanoid located, extracting cheat offsets...\n";
    
    // WalkSpeed - SPEED HACK ESSENTIAL
    for (uintptr_t offset = 0x1C0; offset < 0x220; offset += 4) {
        try {
            float walkSpeed = memory->read<float>(humanoid + offset);
            if (walkSpeed >= 8.0f && walkSpeed <= 50.0f) {
                outputGen.AddOffset("WalkSpeed", offset, true, "humanoid walkspeed (speed hack)", "Humanoid");
                std::cout << std::format("[+] walkspeed at: 0x{:x} (current: {})\n", offset, walkSpeed);
                break;
            }
        } catch (...) {}
    }
    
    // JumpPower/JumpHeight - FLY HACK HELPER
    for (uintptr_t offset = 0x1A0; offset < 0x200; offset += 4) {
        try {
            float jumpPower = memory->read<float>(humanoid + offset);
            if (jumpPower >= 20.0f && jumpPower <= 100.0f) {
                outputGen.AddOffset("JumpPower", offset, true, "humanoid jump power", "Humanoid");
                std::cout << std::format("[+] jumppower at: 0x{:x}\n", offset);
                break;
            }
        } catch (...) {}
    }
    
    // Health - GODMODE HACK
    for (uintptr_t offset = 0x190; offset < 0x1F0; offset += 4) {
        try {
            float health = memory->read<float>(humanoid + offset);
            if (health > 0.0f && health <= 200.0f) {
                outputGen.AddOffset("Health", offset, true, "humanoid health (godmode)", "Humanoid");
                std::cout << std::format("[+] health at: 0x{:x} (current: {})\n", offset, health);
                break;
            }
        } catch (...) {}
    }
    
    // MaxHealth
    for (uintptr_t offset = 0x1B0; offset < 0x210; offset += 4) {
        try {
            float maxHealth = memory->read<float>(humanoid + offset);
            if (maxHealth >= 100.0f && maxHealth <= 200.0f) {
                outputGen.AddOffset("MaxHealth", offset, true, "humanoid max health", "Humanoid");
                break;
            }
        } catch (...) {}
    }
    
    // HipHeight
    for (uintptr_t offset = 0x1A0; offset < 0x1C0; offset += 4) {
        try {
            float hipHeight = memory->read<float>(humanoid + offset);
            if (hipHeight >= 0.0f && hipHeight <= 10.0f) {
                outputGen.AddOffset("HipHeight", offset, true, "humanoid hip height", "Humanoid");
                break;
            }
        } catch (...) {}
    }
    
    // HumanoidState - for fly detection bypass
    for (uintptr_t offset = 0x860; offset < 0x8A0; offset += 8) {
        try {
            uintptr_t state = memory->read<uintptr_t>(humanoid + offset);
            if (state != 0) {
                outputGen.AddOffset("HumanoidState", offset, true, "humanoid state machine", "Humanoid");
                break;
            }
        } catch (...) {}
    }
    
    // Find HumanoidRootPart for teleport
    uintptr_t rootPart = FindChildByName(character, "HumanoidRootPart");
    if (rootPart) {
        DumpPartOffsets(rootPart, true); // true = is root part
    }
}

void DumpPlayerSpecificOffsets(uintptr_t player) {
    std::cout << "[~] digging into player internals...\n";
    
    // UserId - for player identification
    for (uintptr_t offset = 0x250; offset < 0x290; offset += 8) {
        try {
            uint64_t userId = memory->read<uint64_t>(player + offset);
            if (userId > 1000 && userId < 999999999999) {
                outputGen.AddOffset("UserId", offset, true, "player user id", "Player");
                std::cout << std::format("[+] userid at: 0x{:x}\n", offset);
                break;
            }
        } catch (...) {}
    }
    
    // Character pointer - CRUCIAL FOR TELEPORT/FLY
    for (uintptr_t offset = 0x200; offset < 0x280; offset += 8) {
        try {
            uintptr_t character = memory->read<uintptr_t>(player + offset);
            if (character != 0) {
                std::string charName = memory->readstring(memory->read<uintptr_t>(character + g_nameOffset));
                if (charName.length() > 2 && charName != "Players") {
                    outputGen.AddOffset("Character", offset, true, "player character model", "Player");
                    std::cout << std::format("[+] character ptr at: 0x{:x}\n", offset);
                    
                    // Find humanoid in character
                    DumpCharacterOffsets(character);
                    break;
                }
            }
        } catch (...) {}
    }
    
    // Team offset
    for (uintptr_t offset = 0x230; offset < 0x270; offset += 8) {
        try {
            uintptr_t team = memory->read<uintptr_t>(player + offset);
            if (team != 0) {
                outputGen.AddOffset("Team", offset, true, "player team ref", "Player");
                break;
            }
        } catch (...) {}
    }
    
    // DisplayName 
    for (uintptr_t offset = 0x100; offset < 0x140; offset += 8) {
        try {
            std::string displayName = memory->readstring(memory->read<uintptr_t>(player + offset));
            if (!displayName.empty() && displayName.length() > 1) {
                outputGen.AddOffset("DisplayName", offset, true, "player display name", "Player");
                break;
            }
        } catch (...) {}
    }
    
    // CharacterAppearanceId
    for (uintptr_t offset = 0x240; offset < 0x280; offset += 8) {
        try {
            uint64_t appearanceId = memory->read<uint64_t>(player + offset);
            if (appearanceId > 1000) {
                outputGen.AddOffset("CharacterAppearanceId", offset, true, "character appearance id", "Player");
                break;
            }
        } catch (...) {}
    }
}

void DumpPlayerOffsets() {
    std::cout << "[+] time to get player stuff for esp and whatnot...\n";
    
    g_players = FindChildByName(g_datamodel, "Players");
    if (!g_players) {
        std::cout << "[-] players service ain't showing up\n";
        return;
    }
    
    std::cout << "[+] players service located, searching for localplayer...\n";
    
    // LocalPlayer offset - EXPANDED RANGE
    for (uintptr_t offset = 0x100; offset < 0x180; offset += 8) {
        try {
            uintptr_t localPlayer = memory->read<uintptr_t>(g_players + offset);
            if (localPlayer != 0) {
                std::string playerName = memory->readstring(memory->read<uintptr_t>(localPlayer + g_nameOffset));
                if (playerName.length() > 2 && playerName != "Players") {
                    g_localPlayer = localPlayer;
                    outputGen.AddOffset("LocalPlayer", offset, true, "local player ptr", "Player");
                    std::cout << std::format("[+] localplayer found: {} at 0x{:x}\n", playerName, offset);
                    
                    // Dump player-specific stuff
                    DumpPlayerSpecificOffsets(localPlayer);
                    break;
                }
            }
        } catch (...) {}
    }
    
    if (!g_localPlayer) {
        std::cout << "[-] localplayer not found, that's concerning\n";
        outputGen.AddOffset("LocalPlayer", 0, false, "local player ptr", "Player");
    }
}

void DumpWorkspaceOffsets() {
    std::cout << "[+] workspace stuff for world manipulation...\n";
    
    g_workspace = FindChildByName(g_datamodel, "Workspace");
    if (!g_workspace) {
        std::cout << "[-] workspace missing, that's bad\n";
        return;
    }
    
    outputGen.AddOffset("Workspace", g_childrenOffset, true, "workspace service", "Workspace");
    
    // Gravity - ANTI-GRAVITY HACKS
    for (uintptr_t offset = 0x980; offset < 0xA20; offset += 4) {
        try {
            float gravity = memory->read<float>(g_workspace + offset);
            if (gravity > 190.0f && gravity < 200.0f) {
                outputGen.AddOffset("Gravity", offset, true, "workspace gravity (fly hack)", "Workspace");
                std::cout << std::format("[+] gravity at: 0x{:x} (current: {})\n", offset, gravity);
                break;
            }
        } catch (...) {}
    }
    
    // CurrentCamera - AIMBOT ESSENTIAL
    uintptr_t camera = FindChildByName(g_workspace, "Camera");
    if (camera) {
        outputGen.AddOffset("Camera", 0x450, true, "current camera", "Camera");
        DumpCameraOffsets(camera);
    }
    
    // FogEnd - REMOVE FOG HACK
    for (uintptr_t offset = 0x130; offset < 0x160; offset += 4) {
        try {
            float fogEnd = memory->read<float>(g_workspace + offset);
            if (fogEnd > 100.0f && fogEnd < 10000.0f) {
                outputGen.AddOffset("FogEnd", offset, true, "workspace fog end", "Workspace");
                break;
            }
        } catch (...) {}
    }
    
    // FogStart
    for (uintptr_t offset = 0x130; offset < 0x160; offset += 4) {
        try {
            float fogStart = memory->read<float>(g_workspace + offset);
            if (fogStart >= 0.0f && fogStart < 1000.0f) {
                outputGen.AddOffset("FogStart", offset, true, "workspace fog start", "Workspace");
                break;
            }
        } catch (...) {}
    }
}

// ESP ESSENTIALS - scan all players
void DumpESPOffsets() {
    std::cout << "[+] esp offset hunting time...\n";
    
    if (!g_players) {
        std::cout << "[-] no players service, can't scan for esp offsets\n";
        return;
    }
    
    // scan all players for ESP data
    try {
        uintptr_t childrenStart = memory->read<uintptr_t>(g_players + g_childrenOffset);
        uintptr_t children = memory->read<uintptr_t>(childrenStart);
        
        for (int i = 0; i < 50; i++) {
            uintptr_t player = memory->read<uintptr_t>(children + i * 0x10);
            if (player == 0 || player == g_localPlayer) continue;
            
            std::string playerName = memory->readstring(memory->read<uintptr_t>(player + g_nameOffset));
            if (playerName.length() > 2) {
                std::cout << std::format("[~] analyzing player: {} for esp data\n", playerName);
                
                // Character for ESP rendering
                uintptr_t character = FindCharacter(player);
                if (character) {
                    uintptr_t humanoid = FindHumanoid(character);
                    if (humanoid) {
                        // HealthDisplayDistance - ESP health display
                        for (uintptr_t offset = 0x2D0; offset < 0x300; offset += 4) {
                            try {
                                float displayDist = memory->read<float>(humanoid + offset);
                                if (displayDist > 0.0f && displayDist < 1000.0f) {
                                    outputGen.AddOffset("HealthDisplayDistance", offset, true, "health display distance (esp)", "ESP");
                                    break;
                                }
                            } catch (...) {}
                        }
                        
                        // NameDisplayDistance - ESP name display
                        for (uintptr_t offset = 0x2E0; offset < 0x310; offset += 4) {
                            try {
                                float nameDist = memory->read<float>(humanoid + offset);
                                if (nameDist > 0.0f && nameDist < 1000.0f) {
                                    outputGen.AddOffset("NameDisplayDistance", offset, true, "name display distance (esp)", "ESP");
                                    break;
                                }
                            } catch (...) {}
                        }
                        
                        break;
                    }
                }
            }
        }
    } catch (...) {}
}

void DumpAdvancedDataModelOffsets() {
    std::cout << "[+] advanced internal stuff...\n";
    
    // ScriptContext - for script manipulation
    for (uintptr_t offset = 0x3C0; offset < 0x400; offset += 8) {
        try {
            uintptr_t scriptContext = memory->read<uintptr_t>(g_datamodel + offset);
            if (scriptContext != 0) {
                outputGen.AddOffset("ScriptContext", offset, true, "script execution context", "Advanced");
                break;
            }
        } catch (...) {}
    }
    
    // static pointers - these are consistent
    outputGen.AddOffset("VisualEnginePointer", 0x6D931C0, true, "visual engine ptr", "Advanced");
    outputGen.AddOffset("VisualEngine", 0x10, true, "visual engine offset", "Advanced");
    outputGen.AddOffset("VisualEngineToDataModel1", 0x700, true, "visual to datamodel path 1", "Advanced");
    outputGen.AddOffset("VisualEngineToDataModel2", 0x1C0, true, "visual to datamodel path 2", "Advanced");
    outputGen.AddOffset("TaskSchedulerPointer", 0x710ADA0, true, "task scheduler ptr", "Advanced");
    outputGen.AddOffset("JobsPointer", 0x710AF80, true, "jobs ptr", "Advanced");
    outputGen.AddOffset("PlayerConfigurerPointer", 0x7017788, true, "player configurer ptr", "Advanced");
    outputGen.AddOffset("DataModelDeleterPointer", 0x703A760, true, "datamodel deleter ptr", "Advanced");
    outputGen.AddOffset("FakeDataModelPointer", offsets::FakeDataModelPointer, true, "fake datamodel ptr", "Advanced");
    outputGen.AddOffset("FakeDataModelToDataModel", offsets::FakeDataModelToRealDatamodel, true, "fake to real datamodel", "Advanced");
    outputGen.AddOffset("DataModelToRenderView1", 0x1D8, true, "datamodel to renderview 1", "Advanced");
    outputGen.AddOffset("DataModelToRenderView2", 0x8, true, "datamodel to renderview 2", "Advanced");
    outputGen.AddOffset("DataModelToRenderView3", 0x28, true, "datamodel to renderview 3", "Advanced");
}

void DumpGUIOffsets() {
    std::cout << "[+] gui offsets for overlays and stuff...\n";
    
    // GUI positioning - for overlay rendering
    outputGen.AddOffset("FramePositionX", 0x4C0, true, "gui frame x pos", "GUI");
    outputGen.AddOffset("FramePositionY", 0x4C8, true, "gui frame y pos", "GUI");
    outputGen.AddOffset("FramePositionOffsetX", 0x4C4, true, "gui frame x offset", "GUI");
    outputGen.AddOffset("FramePositionOffsetY", 0x4CC, true, "gui frame y offset", "GUI");
    outputGen.AddOffset("FrameRotation", 0x190, true, "gui frame rotation", "GUI");
    outputGen.AddOffset("FrameSizeX", 0x120, true, "gui frame x size", "GUI");
    outputGen.AddOffset("FrameSizeY", 0x124, true, "gui frame y size", "GUI");
    outputGen.AddOffset("ViewportSize", 0x2F0, true, "viewport size", "GUI");
    outputGen.AddOffset("InsetMinX", 0x100, true, "gui inset min x", "GUI");
    outputGen.AddOffset("InsetMinY", 0x104, true, "gui inset min y", "GUI");
    outputGen.AddOffset("InsetMaxX", 0x108, true, "gui inset max x", "GUI");
    outputGen.AddOffset("InsetMaxY", 0x10C, true, "gui inset max y", "GUI");
}

void DumpMiscOffsets() {
    std::cout << "[+] misc stuff that might be useful...\n";
    
    // String and memory management
    outputGen.AddOffset("StringLength", 0x10, true, "string length field", "Misc");
    outputGen.AddOffset("NameSize", 0x10, true, "name size field", "Misc");
    outputGen.AddOffset("InstanceCapabilities", 0x380, true, "instance capabilities", "Misc");
    outputGen.AddOffset("ModelInstance", 0x328, true, "model instance ref", "Misc");
    outputGen.AddOffset("OnDemandInstance", 0x30, true, "ondemand instance", "Misc");
    outputGen.AddOffset("AttributeList", 0x38, true, "attribute list", "Misc");
    outputGen.AddOffset("AttributeList2", 0x18, true, "secondary attribute list", "Misc");
    outputGen.AddOffset("AttributeToNext", 0x58, true, "attribute to next ptr", "Misc");
    outputGen.AddOffset("AttributeToValue", 0x18, true, "attribute to value ptr", "Misc");
    outputGen.AddOffset("Value", 0xD8, true, "value object data", "Misc");
    outputGen.AddOffset("Deleter", 0x10, true, "instance deleter", "Misc");
    outputGen.AddOffset("DeleterBack", 0x18, true, "deleter back ref", "Misc");
    outputGen.AddOffset("Primitive", 0x178, true, "primitive data", "Misc");
    outputGen.AddOffset("PrimitiveGravity", 0x120, true, "primitive gravity", "Misc");
    outputGen.AddOffset("PrimitiveValidateValue", 0x6, true, "primitive validation", "Misc");
    outputGen.AddOffset("PrimitivesPointer1", 0x3D8, true, "primitives ptr 1", "Misc");
    outputGen.AddOffset("PrimitivesPointer2", 0x210, true, "primitives ptr 2", "Misc");
    outputGen.AddOffset("DataModelPrimitiveCount", 0x410, true, "datamodel primitive count", "Misc");
    outputGen.AddOffset("WorkspaceToWorld", 0x3D8, true, "workspace to world ptr", "Misc");
    outputGen.AddOffset("viewmatrix", 0x4B0, true, "view matrix", "Misc");
    outputGen.AddOffset("Dimensions", 0x720, true, "object dimensions", "Misc");
    outputGen.AddOffset("CreatorId", 0x190, true, "creator user id", "Misc");
}

int main() {
    SetConsoleTitleA("Offset Dumper");
    
    std::cout << "offset dumper\n";
    std::cout << "===================================\n";
    std::cout << "yo what's good, bout to extract every offset needed for cheats\n";
    std::cout << "generated by: nwesk | " << GetRobloxVersion() << "\n\n";
    
    std::filesystem::create_directories("output");
    
    attach();
    if (!base_address) return -1;
    
    outputGen.SetRobloxVersion(GetRobloxVersion());
    outputGen.SetByfronVersion("still unknown lol");
    
    g_datamodel = getdatamodel();
    std::cout << std::format("[+] datamodel secured at: 0x{:X}\n", g_datamodel);
    
    // wait a bit for game to fully load
    std::cout << "[~] giving the game a sec to load properly...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "aight let's extract these cheat offsets fr\n";
    std::cout << std::string(50, '=') << "\n";
    
    DumpBasicOffsets();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    DumpPlayerOffsets();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    DumpWorkspaceOffsets();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    DumpESPOffsets();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    DumpAdvancedDataModelOffsets();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    DumpGUIOffsets();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    DumpMiscOffsets();
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "cooking up the output files...\n";
    std::cout << std::string(50, '=') << "\n";
    
    outputGen.GenerateOutput();
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "we done here chief, got the goods\n";
    std::cout << "found: " << outputGen.GetFoundCount() << "/" << outputGen.GetTotalCount() << " offsets\n";
    std::cout << "success rate: " << (outputGen.GetFoundCount() * 100 / outputGen.GetTotalCount()) << "%\n";
    std::cout << "generated by: nwesk | keep it lowkey\n";
    std::cout << std::string(50, '=') << "\n";

    std::cout << "\npress any key to dip...\n";
    std::cin.get();
    return 0;
}