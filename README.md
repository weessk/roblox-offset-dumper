# offset dumper 🕵️‍♂️

yo what dis is:  
just a lil dumper 4 roblox offsets cuz im tired of ppl sellin sum shit that takes 5 min to scan 💀  
keeps it simple, keeps it lowkey, spits out headers / txt / json.  

> side note: had this proj sittin for a while, just droppin it public now 🤷‍♂️  

⚠️ heads up: recommended to run on alt / multi acc, roblox be handin out bans sometimes 🚫  

## 📂 output
when u run it u get:
- `output/offsets.hpp` → c++ header (plug straight in ur cheat)  
- `output/offsets.txt` → readable list, for da homies  
- `output/offsets.json` → if u tryna feed it somewhere else  

## ⚡ usage
1. edit [`offsets.hpp`](offsets.hpp) → set ur **placeId**  
   ```cpp
   inline constexpr uintptr_t PLACE_ID = 168556275; // ur place here ```
    
2. run `build.bat` → it’ll call **g++** (mingw) n spit out `offset-dumper.exe`
3. open **roblox** (make sure ur target exp is up)
4. run `offset-dumper.exe` → check `/output` folder for ur loot

## 🔧 deps

* windows only (uses winapi / psapi)
* g++ (mingw / gcc) installed & on ur PATH
* c++17 or higher (no oldhead compiler plz)

## 💿 how it works

* attach → scan → dump
* organizes offsets by category
* auto timestamps + roblox/byfron ver tags
* lil success rate summary (so u kno if u cooked or nah)

## 🧩 credits

* made by nwesk (keep it lowkey fr)
* if u skid it, at least rename it clown 🤡

---

> disclaimer: edu stuff only bruh, don’t get ur acc clapped
