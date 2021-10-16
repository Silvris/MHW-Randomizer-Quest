// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Windows.h"
#include <Psapi.h>
#include "json/json.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <random>
#include <thread>

#include "deps/loader.h"
#include "deps/mhw_console.h"

using namespace loader;
using json = nlohmann::json;

typedef unsigned char byte;

//Credits to Strackeror for his scanmem and protect/unprotect functions, saved me the worst of the pain for writing this

__declspec(dllexport) void load() {};

typedef unsigned char byte;
static char PLUGIN_KEY[] = "randomizer ";
static std::string PLUGIN_NAME = "Randomizer";


template<typename T>
bool unprotect(T ptr, int len, PDWORD oldp) {
    return VirtualProtect((LPVOID)(ptr), len, PAGE_EXECUTE_READWRITE, oldp);
}

template<typename T>
bool protect(T ptr, int len, PDWORD oldp) {
    DWORD dummy;
    return VirtualProtect((LPVOID)(ptr), len, *oldp, &dummy);
}

bool apply(byte* ptr, std::vector<byte> replace) {
    DWORD protection;
    if (!unprotect(ptr, replace.size(), &protection)) {
        return false;
    }
    if (!memcpy(ptr, &replace[0], replace.size())) {
        return false;
    }
    if (!protect(ptr, replace.size(), &protection)) {
        return false;
    }
    return true;
}

template<typename T, typename Q>
bool apply(T* ptr, Q replace) {
    DWORD protection;
    if (!unprotect(ptr,sizeof(replace), &protection)) {
        return false;
    }
    if (!memcpy((byte *) ptr, (byte *) &replace, sizeof(replace))) {
        return false;
    }
    if (!protect(ptr, sizeof(replace), &protection)) {
        return false;
    }
    return true;
}

struct Monster {
    std::string name;
    int monsterId;
    std::vector<int> sobjs;
};

struct Quests {
    long long* oneMonPtr;
    long long* twoMonPtr;
    long long* threeMonPtr;
    long long* fourMonPtr;
    long long* fiveMonPtr;
    long long* ranMonPtr;
    std::vector<Monster> monsters;
};

struct Options {
    int healthMin = 0;
    int healthMax = 0;
    int attackMin = 0;
    int attackMax = 0;
    int defenseMin = 0;
    int defenseMax = 0;
    int partHPMin = 0;
    int partHPMax = 0;
    int initValuesMin = 0;
    int initValuesMax = 0;
};

static std::string stageToName(int stageId) {
    switch (stageId) {
    case 201:
        return "Special Arena";
    case 202:
        return "Challenge Arena";
    case 203:
        return "Seliana Supply Cache";
    case 412:
        return "Origin Isle (Small)";
    case 413:
        return "Origin Isle (Large)";
    case 416:
        return "Secluded Valley";
    case 417:
        return "Castle Schrade";
    default:
        return "ERROR";
    }
}

static void RunRandomizer(long long* questOff, std::vector<Monster> monsters, std::default_random_engine &gen, std::ofstream& log, Options opts) {
    static int stages[7] = { 201,202,203,412,413,416,417 };
    std::uniform_int_distribution<int> monsterCnt(1, 5);
    std::uniform_int_distribution<int> monsterGen(0,(monsters.size()-1));//monster generator
    std::uniform_int_distribution<int> health(opts.healthMin, opts.healthMax);//300% - 1300%,tried to just get to close max, sadly lots of bleh values in the lot
    std::uniform_int_distribution<int> attack(opts.attackMin, opts.attackMax);//~300% - 1200%, not as clean as others though as many slots just jump around
    std::uniform_int_distribution<int> defense(opts.defenseMin, opts.defenseMax);//90% - 110%
    std::uniform_int_distribution<int> partHP(opts.partHPMin, opts.partHPMax);//70% - 540%, with a bonus 1000% in there
    std::uniform_int_distribution<int> initValues(opts.initValuesMin, opts.initValuesMax);//70% - 500%, Status/StatusBuildup/KO/Exhaust/Mount all use identical values in this range, let's me save some line of code
    std::uniform_int_distribution<int> stageVals(0, 6);//static value because any added stages will require an added update for new files
    //size will not be edited for this, as I'd rather not deal with figuring out how to define "proper" sizes for each monster
    int* questId = (int*)(*questOff + 0x120);//grab quest Id for logic
    int* monsterOffset = (int*)(*questOff + 0x190);//this is further iterated by monster slot
    int monsterNum = 0;
    switch (*questId) {
    case 99994:
        monsterNum = monsterCnt(gen);
        break;
    case 99995:
        monsterNum = 1;
        break;
    case 99996:
        monsterNum = 2;
        break;
    case 99997:
        monsterNum = 3;
        break;
    case 99998:
        monsterNum = 4;
        break;
    case 99999:
        monsterNum = 5;
        break;
    default:
        monsterNum = 1;
        break;
    }
    if (*questId == 99994) {
        //randomize stage here
        int* stageId = (int*)((byte*)*questOff + 0x134);
        int genStage = stages[stageVals(gen)];
        log << "Stage: " << stageToName(genStage) << std::endl << std::endl;
        apply(stageId, genStage);
    }
    if (monsterNum > 0) {
        int* idOffset = monsterOffset;
        int* sobjOffset = (int*)((byte*)monsterOffset + 0x1C);
        int* healthOffset = (int*)((byte*)monsterOffset + 0x54);
        int* attackOffset = (int*)((byte*)monsterOffset + 0x70);
        int* defenseOffset = (int*)((byte*)monsterOffset + 0xFC);
        int* partHPOffset = (int*)((byte*)monsterOffset + 0x118);
        int* statusOffset = (int*)((byte*)monsterOffset + 0x134);
        int* statusBuildupOffset = (int*)((byte*)monsterOffset + 0x150);
        int* stunOffset = (int*)((byte*)monsterOffset + 0x16C);
        int* exhaustOffset = (int*)((byte*)monsterOffset + 0x188);
        int* mountOffset = (int*)((byte*)monsterOffset + 0x1A4);
        for (int i = 0; i < monsterNum; i++) {
            //now iterate over each value
            Monster currentMon = monsters[monsterGen(gen)];
            //setup quest goals
            if (monsterNum < 3 && i == 0) {
                //done this way so it is edited for both 1 and 2 mon quests, but not 3+
                byte* qFlags = (byte*)*questOff + 0xC0;
                short* goal1 = (short*)((byte*)*questOff + 0x140);
                apply(qFlags, 1);
                apply(goal1, 49);
                short* target1 = (short*)((byte*)*questOff + 0x150);
                short* amount1 = (short*)((byte*)*questOff + 0x158);
                apply(target1, currentMon.monsterId);
                apply(amount1, 1);
            }
            else if (monsterNum < 3 && i == 1) {
                short* target2 = (short*)((byte*)*questOff + 0x152);
                short* amount2 = (short*)((byte*)*questOff + 0x15A);
                apply(target2, currentMon.monsterId);
                apply(amount2, 1);
            }
            else if (monsterNum >= 3 && i == 0) {
                byte* qFlags = (byte*)*questOff + 0xC0;
                short* goal1 = (short*)((byte*)*questOff + 0x140);
                apply(qFlags, 16);
                apply(goal1, 1025);
                short* target1 = (short*)((byte*)*questOff + 0x150);
                short* amount1 = (short*)((byte*)*questOff + 0x158);
                short* target2 = (short*)((byte*)*questOff + 0x152);
                short* amount2 = (short*)((byte*)*questOff + 0x15A);
                apply(target1, 0);
                apply(amount1, monsterNum);
                apply(target2, 0);
                apply(amount2, 0);
            }
            //create distribution for sobjs
            std::uniform_int_distribution<int> sobjDist(0, (currentMon.sobjs.size()-1));
            //now do actual monster info
            //generate values to variables so that they can be written out to log
            int sobj = currentMon.sobjs[sobjDist(gen)];
            int monHealth = health(gen);
            int monAttack = attack(gen);
            int monDefense = defense(gen);
            int monPartHP = partHP(gen);
            int monStatus = initValues(gen);
            int monStatusBuild = initValues(gen);
            int monStun = initValues(gen);
            int monExhaust = initValues(gen);
            int monMount = initValues(gen);
            log << "Monster: " << currentMon.name << std::endl;
            log << "SOBJ: " << sobj << std::endl;
            log << "Health: " << monHealth << std::endl;
            log << "Attack: " << monAttack << std::endl;
            log << "Defense: " << monDefense << std::endl;
            log << "Part HP: " << monPartHP << std::endl;
            log << "Status: " << monStatus << std::endl;
            log << "Status Buildup: " << monStatusBuild << std::endl;
            log << "Stun: " << monStun << std::endl;
            log << "Exhaust: " << monExhaust << std::endl;
            log << "Mount: " << monMount << std::endl << std::endl;
            apply(&idOffset[i], currentMon.monsterId);
            apply(&sobjOffset[i], sobj);
            apply(&healthOffset[i], monHealth);
            apply(&attackOffset[i], monAttack);
            apply(&defenseOffset[i], monDefense);
            apply(&partHPOffset[i], monPartHP);
            apply(&statusOffset[i], monStatus);
            apply(&statusBuildupOffset[i], monStatusBuild);
            apply(&stunOffset[i], monStun);
            apply(&exhaustOffset[i], monExhaust);
            apply(&mountOffset[i], monMount);
        }
        if (*questId == 99994 && monsterNum < 5) {
            for (int i = 0; i < (5 - monsterNum); i++) {
                //remove unwanted monsters from quests
                apply(&idOffset[4 - i], -1);
            }
        }
    }

}

std::string random_string(size_t length)
{
    auto randchar = []() -> char
    {
        const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
}//copied from StackOverflow, by user Carl

static Quests Initialize(Options &opts) {
    Quests qData{};
    std::vector<long long*> questOffsets;
    auto module = GetModuleHandleA("MonsterHunterWorld.exe");
    if (module == nullptr) return qData;

    byte* startAddr = (byte*)module;
    long* questArrPtr = (long*)(startAddr + 0x506F240);
    //LOG(INFO) << std::hex << questArrPtr;
    //in hind-sight, how does this work
    long* questArrAddr = (long*)(*questArrPtr + 0xAEE8);//C++ will give you an warning here, but changing it breaks functionality
    //LOG(INFO) << std::hex << questArrAddr;
     //now get the list count from it's pointer
    
    long* questNoPtr = (long*)(startAddr + 0x5073258);
    int* questNum = (int*)(*questNoPtr + 184);
    //This was initially used, but then I found out that the actual entries is larger than the quest count normally, but 8192 bytes are allocated for it
    //looking at the ids, it's likely that the discrepency is created by the event quests not formally being part of QuestNoList
    int index = 0;
    for (int i = 0; i < *questNum; i++) {
        long long* questAddr = (long long*)(questArrAddr + (index));
        //LOG(INFO)<< i << " " << std::hex << questAddr << " " << *questAddr;
        if (*questAddr != 0) {
            questOffsets.push_back(questAddr);
        }
        index += 2;
    }
    
    for (int i = 0; i < questOffsets.size(); i++) {
        int* questId = (int*)(*questOffsets[i] + 0x120);
        //LOG(INFO) << *questId;
        switch (*questId) {
        case 99994:
            qData.ranMonPtr = questOffsets[i];
            break;
        case 99995:
            qData.oneMonPtr = questOffsets[i];
            break;
        case 99996:
            qData.twoMonPtr = questOffsets[i];
            break;
        case 99997:
            qData.threeMonPtr = questOffsets[i];
            break;
        case 99998:
            qData.fourMonPtr = questOffsets[i];
            break;
        case 99999:
            qData.fiveMonPtr = questOffsets[i];
            break;
        default:
            break;
        }
    }
    if (qData.oneMonPtr&&qData.twoMonPtr&&qData.threeMonPtr&&qData.fourMonPtr&&qData.fiveMonPtr&&qData.ranMonPtr) {
        LognSend("Randomzier Quests found!");
    }
    else {
        LognSend("Unable to locate all quests. Try initializing again.");
        
    }

    // now initialize monster Data within qData
    // this means players can also edit what mons the rando uses while the game is running
    // by just reinitializing
    std::string root;
    if (std::filesystem::exists("ICE")) { root = "ICE/ntPC"; }
    else { root = "nativePC"; }
    std::string path(root + "/plugins/RandomizerMonsters");
    std::string ext(".json");
    for (auto& p : std::filesystem::recursive_directory_iterator(path)) {
        if (p.path().extension() == ext) {
            //LOG(INFO) << p.path();
            std::ifstream file(p.path());
            if (file.fail()) {
                //LognSend("Failed to open monster file:");
                //LOG(INFO) << p.path();
            }
            json mon = json::object();
            file >> mon;
            Monster newMon;
            newMon.monsterId = mon["monsterId"];
            newMon.name = mon["name"];
            for (auto obj : mon["sobjs"]) {
                newMon.sobjs.push_back(obj["sobj"]);
            }
            qData.monsters.push_back(newMon);
            //LognSend("Loaded monster: " + newMon.name);
            file.close();
        }
    }
    LognSend("Loaded " + std::to_string(qData.monsters.size()) + " monsters!");
    // read options file finally
    std::ifstream file("nativePC/plugins/RandomizerFiles/randomizerOptions.json");
    if (file.fail()) {
        LognSend("Options file unable to be read");
    }
    json options = json::object();
    file >> options;
    opts.healthMin = options["healthMin"];
    opts.healthMax = options["healthMax"];
    opts.attackMin = options["attackMin"];
    opts.attackMax = options["attackMax"];
    opts.defenseMin = options["defenseMin"];
    opts.defenseMax = options["defenseMax"];
    opts.partHPMin = options["partHPMin"];
    opts.partHPMax = options["partHPMax"];
    opts.initValuesMin = options["initValuesMin"];
    opts.initValuesMax = options["initValuesMax"];
    file.close();
    return qData;
}

static DWORD WINAPI MainLoop(LPVOID lpParam) {
    std::srand(time(NULL));//for seed generation
    while (!ConsoleEnable) {
        for (int i = 0; i < 1000; i++) {
            i++;
        }
        ConsoleEnable = TCPConnect();
    }
    char* ctx = NULL;
    //Quests struct holds pointers for each quest
    Quests quests;
    //Options struct to hold option data
    Options opts;
    //seed stored statically to allow for copying
    std::string seed = "";
    //should always be active as long as console connection works
    while (ConsoleEnable) {

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(s, buffer, 1024, 0);
        std::string command(buffer);
        //LognSend("Test");
        if (command != "") {
            //command received, now see if its one for us
            if (strncmp(buffer, PLUGIN_KEY, strlen(PLUGIN_KEY)) == 0 && strlen(buffer) >= strlen(PLUGIN_KEY) + 1)
            {
                char* pch;
                pch = strtok_s(buffer, " ",&ctx);
                pch = strtok_s(NULL, " ",&ctx);

                // Globally turn on/off plugin, mostly for keyboard keys
                if (strncmp(pch, "init", 4) == 0)
                    {
                        // Reload plugin
                        LognSend("[" + PLUGIN_NAME + "] [Console] Initializing...");
                        quests = Initialize(opts);
                    }
                else if (strncmp(pch, "rand", 4) == 0) 
                    {
                        pch = strtok_s(NULL, " ", &ctx);
                        
                        if (pch != NULL) {
                            //seed is given to us
                             seed = std::string(pch);
                        }
                        else {
                            //generate new seed
                            seed = random_string(23);//23 is the remaining amount of characters we "should" have in the buffer, might actually be different if Unicode characters are used
                            
                        }
                        LognSend("Using seed: " + seed);
                        std::ofstream logFile("nativePC/plugins/RandomizerFiles/randomizerLog.txt");
                        logFile << "Seed: " + seed << std::endl << std::endl;
                        std::seed_seq seedSeq(seed.begin(), seed.end());
                        std::default_random_engine gen(seedSeq);
                        if (quests.oneMonPtr) {
                            logFile << "One Monster:" << std::endl;
                            RunRandomizer(quests.oneMonPtr, quests.monsters, gen, logFile, opts);
                            logFile << std::endl;
                        }
                        if (quests.twoMonPtr) {
                            logFile << "Two Monsters:" << std::endl;
                            RunRandomizer(quests.twoMonPtr, quests.monsters, gen, logFile, opts);
                            logFile << std::endl;
                        }
                        if (quests.threeMonPtr) {
                            logFile << "Three Monsters:" << std::endl;
                            RunRandomizer(quests.threeMonPtr, quests.monsters, gen, logFile, opts);
                            logFile << std::endl;
                        }
                        if (quests.fourMonPtr) {
                            logFile << "Four Monsters:" << std::endl;
                            RunRandomizer(quests.fourMonPtr, quests.monsters, gen, logFile, opts);
                            logFile << std::endl;
                        }
                        if (quests.fiveMonPtr) {
                            logFile << "Five Monsters:" << std::endl;
                            RunRandomizer(quests.fiveMonPtr, quests.monsters, gen, logFile, opts);
                            logFile << std::endl;
                        }
                        if (quests.ranMonPtr) {
                            logFile << "Random Monsters:" << std::endl;
                            RunRandomizer(quests.ranMonPtr, quests.monsters, gen, logFile, opts);
                            logFile << std::endl;
                        }
                        
                    }
                else if (strncmp(pch, "copy", 4) == 0) {
                    HWND hwnd = GetDesktopWindow();//have to call this so we get the right clipboard
                    OpenClipboard(hwnd);
                    EmptyClipboard();
                    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, seed.size() + 1);
                    if (!hg) {
                        LognSend("Failed to copy to clipboard.");
                        CloseClipboard();
                    }
                    else {
                        memcpy(GlobalLock(hg), seed.c_str(), (seed.size() + 1));
                        GlobalUnlock(hg);
                        SetClipboardData(CF_TEXT, hg);
                        CloseClipboard();
                        GlobalFree(hg);
                        LognSend("Copied to clipboard!");
                    }
                    
                }

                Sleep(500);
            }
        }
    }
    return 0;
}

static void onLoad() {
    DWORD thread;
    CreateThread(NULL, 0, MainLoop, 0, 0, &thread);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        onLoad();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

