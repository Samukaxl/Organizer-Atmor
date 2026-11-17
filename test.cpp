#include "core.hpp"
#include "transaction.hpp"
#include "online.hpp"
#include <cassert>
#include <iostream>
#include <random>
int main(int argc,char**argv){
 auto data=readFile(argc>1?argv[1]:"fonte/modlist-referencia.txt");auto rules=parseRules(readFile("regras.tsv"));auto plan=organize(data,{},rules,false);
 assert(plan.p.mods.size()==263&&plan.unknown==0);assert(modEntries(plan.serialized)==modEntries(data));assert(!plan.separators.empty());assert(organize(plan.serialized,{},rules,false).serialized==plan.serialized);
 assert(inferCategory("Novo mod",{"interface/test.swf"}).cat==12);assert(inferCategory("Vozes novas",{"sound/voice/a.esp/actor/a.fuz"}).cat==11);assert(inferCategory("PT-BR voice",{"sound/voice/a.esp/actor/a.fuz"}).cat==15);assert(inferCategory("Unknown",{"textures/new.dds"}).cat==1);assert(inferCategory("Unknown",{"mysterious.esp"}).cat==17);
 auto src=sourceFromMeta("[General]\nmodid=123\ngameName=Skyrim Special Edition\n");assert(src.valid()&&src.game=="skyrimspecialedition"&&src.id=="123");assert(!sourceFromMeta("[General]\nmodid=../123\ngameName=SkyrimSE\n").valid());
 auto page=parsePage("<html><title>Fallback</title><meta content='Mod A &amp; B' property='og:title'><meta name=\"description\" content=\"A useful mod\"></html>");assert(page.title=="Mod A & B"&&page.description=="A useful mod");assert(parsePage("<title>Just a moment...</title>").title.empty());
 // Windows path normalization can be tested as pure strings on the host.
 const std::wstring originalPath=L"C:\\Users\\teste\\mods\\animação_separator\\meta.ini";
 const std::wstring extendedPath=L"\\\\?\\C:\\Users\\teste\\mods\\animação_separator\\meta.ini";
 assert(extendedWindowsAbsolute(originalPath)==extendedPath);
 assert(extendedWindowsAbsolute(extendedPath)==extendedPath);
 assert(extendedWindowsAbsolute(L"C:/Users/teste/mods/animação_separator/meta.ini")==extendedPath);
 assert(extendedWindowsAbsolute(L"\\\\server\\share\\mods")==L"\\\\?\\UNC\\server\\share\\mods");
 auto base=fs::temp_directory_path()/"samukxll-test-v05";fs::remove_all(base);auto root=base/"mods";fs::create_directories(root/"A"/"textures");fs::create_directories(root/"B"/"textures");fs::create_directories(root/"U");writeFile(root/"A"/"textures"/"same.dds","a");writeFile(root/"B"/"textures"/"same.dds","b");
 auto c=organize("+B\n+A\n",root,{});assert(c.order==vector<int>({0,1})&&c.shared==1&&c.conflicts[0].winner=="B");auto off=organize("-B\n+A\n",root,{});assert(off.conflicts.empty());assert(modEntries(off.serialized)==modEntries("-B\n+A\n"));
 bool threw=false;try{parse("+A\n-A\n");}catch(...){threw=true;}assert(threw);threw=false;try{organize("+Missing\n",root,{});}catch(const std::exception&e){threw=true;assert(string(e.what()).find("Missing")!=string::npos);}assert(threw);threw=false;try{parse("+../bad\n");}catch(...){threw=true;}assert(threw);
 auto deep=root/"A";for(int i=0;i<5;i++)deep/=string(65,'x')+std::to_string(i);fs::create_directories(deep);writeFile(deep/fs::u8path("ação.dds"),"x");fs::create_directories(root/"A"/"hidden.mohidden");writeFile(root/"A"/"hidden.mohidden"/"skip.dds","x");writeFile(root/"A"/"meta.ini","metadata");
 bool deepFound=false;for(auto&f:scanFiles(root/"A")){assert(f.find("hidden.mohidden")==string::npos&&f!="meta.ini");if(f.find("ação.dds")!=string::npos){deepFound=true;assert(f.size()>300);}}assert(deepFound);
#ifndef _WIN32
 fs::create_symlink(root/"A"/"meta.ini",root/"A"/"linked.ini");threw=false;try{scanFiles(root/"A");}catch(...){threw=true;}assert(threw);fs::remove(root/"A"/"linked.ini");
#endif
 std::mt19937 gen(42);for(int trial=0;trial<60;trial++){string input;Rules r;for(int i=0;i<200;i++){string name="M"+std::to_string(i);input=(gen()%4?"+":"-")+name+"\n"+input;r[name]=gen()%18;}auto p=organize(input,{},r,false);assert(modEntries(p.serialized)==modEntries(input));assert(!p.separators.empty()&&p.separators.size()<=p.p.mods.size());assert(organize(p.serialized,{},r,false).serialized==p.serialized);}
 // A long sequence of distinct substantial categories is not capped at eight.
 string longList;Rules distinct;for(int block=0;block<12;block++)for(int j=0;j<12;j++){string name="Block"+std::to_string(block)+"Mod"+std::to_string(j);longList="+"+name+"\n"+longList;distinct[name]=block%2?1:0;}
 assert(organize(longList,{},distinct,false).separators.size()>8);
 // Reordering is constrained by named rules, not arbitrary category sorting.
 fs::create_directories(root/"Example Quest");fs::create_directories(root/"Example Quest PT-BR");
 writeFile(root/"Example Quest"/"quest.esp","placeholder");writeFile(root/"Example Quest PT-BR"/"quest.esp","translated");
 auto proposed=organize("+Example Quest\n+Example Quest PT-BR\n",root,{},true,{},true);
 assert(proposed.moved==2&&proposed.p.mods.back().name=="Example Quest PT-BR");assert(proposed.conflicts[0].winner=="Example Quest PT-BR");assert(organize(proposed.serialized,root,{},true,{},true).serialized==proposed.serialized);
 auto unchanged=organize("+Example Quest\n+Example Quest PT-BR\n",root,{},true,{},false);assert(unchanged.moved==0);
 auto manual=parsePriorities("Example Quest PT-BR\tExample Quest\tExplicit user choice\n");auto overridden=organize("+Example Quest\n+Example Quest PT-BR\n",root,{},true,manual,true);assert(overridden.moved==0);
 bool cycle=false;try{organize("+Example Quest\n+Example Quest PT-BR\n",root,{},true,{{"Example Quest","Example Quest PT-BR","one"},{"Example Quest PT-BR","Example Quest","two"}},true);}catch(...){cycle=true;}assert(cycle);
 auto accent=root/fs::u8path("animação_separator");makeSeparator(root,{"animação_separator",string("[General]\ncolor=123\n")});assert(separatorMetadata(accent)==std::optional<string>("[General]\ncolor=123\n"));removeSeparator(root,{"animação_separator",string("[General]\ncolor=123\n")});
 // Exercise separator cleanup, shared profiles, rollback at every mutation stage,
 // full restoration, and the restoration of a restoration.
 auto profile=base/"profiles"/"Default"/"modlist.txt",other=base/"profiles"/"Other"/"modlist.txt";fs::create_directories(profile.parent_path());fs::create_directories(other.parent_path());
 const string original="# Original\r\n+B\r\n-old2_separator\r\n+A\r\n-old_separator\r\n",otherData="# Other\r\n-A\r\n+B\r\n-old_separator\r\n";
 for(int fail=1;fail<=4;fail++){
  for(auto&e:fs::directory_iterator(root))if(ends(e.path().filename().u8string(),"_separator"))fs::remove_all(e.path());
  makeSeparator(root,{"old_separator",string("[General]\ncolor=1\n")});makeSeparator(root,{"old2_separator",std::nullopt});writeFile(profile,original);writeFile(other,otherData);
  auto p=organize(original,root,{});auto clean=planCleanup(root,profile,p);assert(clean.retired.size()==2&&clean.edits.size()==2);vector<SeparatorData>ensure;for(auto&n:p.separators)ensure.push_back({n,string("[General]\n")});auto backup=base/("backup"+std::to_string(fail));
  threw=false;try{executeTransaction(root,profile,backup,clean.edits,clean.retired,ensure,[&](int stage){if(stage==fail)throw std::runtime_error("Injected test failure");});}catch(...){threw=true;}
  if(fail<=3){assert(threw&&readFile(profile)==original&&readFile(other)==otherData);assert(separatorMetadata(root/"old_separator")==std::optional<string>("[General]\ncolor=1\n"));assert(fs::is_directory(root/"old2_separator"));for(auto&n:p.separators)assert(!fs::exists(root/fs::u8path(n)));}
  else{assert(!threw&&readFile(profile)==p.serialized);assert(modEntries(readFile(other))==modEntries(otherData));assert(!fs::exists(root/"old_separator"));restoreV4(backup,base/"safety");assert(readFile(profile)==original&&readFile(other)==otherData);assert(separatorMetadata(root/"old_separator")==std::optional<string>("[General]\ncolor=1\n"));for(auto&n:p.separators)assert(!fs::exists(root/fs::u8path(n)));restoreV4(base/"safety",base/"safety2");assert(readFile(profile)==p.serialized);}
 }
 writeFile(profile,"+Example Quest\n+Example Quest PT-BR\n");auto beforePriority=readFile(profile);auto priorityPlan=organize(beforePriority,root,{},true,{},false,true);auto priorityCleanup=planCleanup(root,profile,priorityPlan);vector<SeparatorData>priorityEnsure;for(auto&name:priorityPlan.separators)priorityEnsure.push_back({name,string("[General]\n")});executeTransaction(root,profile,base/"priority-backup",priorityCleanup.edits,priorityCleanup.retired,priorityEnsure);assert(readFile(profile)==priorityPlan.serialized);restoreV4(base/"priority-backup",base/"priority-safety");assert(readFile(profile)==beforePriority);
 makeSeparator(root,{"protected_separator",std::nullopt});writeFile(root/"protected_separator"/"user-note.txt","Keep me");threw=false;try{planCleanup(root,profile,organize(readFile(profile),root,{}));}catch(...){threw=true;}assert(threw&&readFile(root/"protected_separator"/"user-note.txt")=="Keep me");
 fs::remove_all(base);std::cout<<"PASS: exact order/states, adaptive blocks, 60 randomized lists, classification, conflicts, source parsing, long/Unicode paths, separator archival, shared profiles, rollback, restore and protected data.\n";
 writeFile("previa-da-sua-lista.txt",plan.serialized);std::cout<<"Reference list: "<<plan.separators.size()<<" separators for "<<plan.p.mods.size()<<" mods.\n";
}
