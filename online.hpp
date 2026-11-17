#pragma once
#include "core.hpp"
#include <atomic>
#include <cstring>
#include <chrono>
#include <mutex>
#include <cctype>
#include <thread>
#ifdef _WIN32
#include <winhttp.h>
#endif
struct Source{string game,id;string url()const{return "https://www.nexusmods.com/"+game+"/mods/"+id;}bool valid()const{return !game.empty()&&!id.empty();}};
inline string trim(string s){auto a=s.find_first_not_of(" \t\r\n\"");if(a==string::npos)return "";auto b=s.find_last_not_of(" \t\r\n\"");return s.substr(a,b-a+1);}
inline Source sourceFromMeta(const string&data){string game,id;bool general=false;for(auto&line:lines(data)){auto l=trim(line);if(l.empty())continue;if(l[0]=='['){general=low(l)=="[general]";continue;}if(!general)continue;auto eq=l.find('=');if(eq==string::npos)continue;auto key=low(trim(l.substr(0,eq))),val=trim(l.substr(eq+1));if(key=="gamename")game=low(val);if(key=="modid")id=val;}
 const std::map<string,string>games={{"skyrim special edition","skyrimspecialedition"},{"skyrimse","skyrimspecialedition"},{"skyrimspecialedition","skyrimspecialedition"},{"skyrim","skyrim"},{"fallout 4","fallout4"},{"fallout4","fallout4"},{"fallout new vegas","newvegas"},{"falloutnv","newvegas"},{"oblivion","oblivion"}};
 auto g=games.find(game);if(g==games.end()||id.empty()||id.size()>10||id.find_first_not_of("0123456789")!=string::npos||id.find_first_not_of('0')==string::npos)return {};return {g->second,id};
}
inline string plainText(const string&input){string s;bool tag=false;for(char c:input){if(c=='<'){tag=true;continue;}if(c=='>'){tag=false;s+=' ';continue;}if(!tag)s+=c;if(s.size()>=1500)break;}
 const std::pair<const char*,const char*>entities[]={{"&amp;","&"},{"&quot;","\""},{"&#39;","'"},{"&apos;","'"},{"&lt;","<"},{"&gt;",">"},{"&nbsp;"," "}};for(auto&[a,b]:entities){size_t pos=0;while((pos=s.find(a,pos))!=string::npos){s.replace(pos,strlen(a),b);pos+=strlen(b);}}for(char&c:s)if(c=='\r'||c=='\n'||c=='\t')c=' ';
#ifdef _WIN32
 int n=MultiByteToWideChar(CP_UTF8,0,s.data(),s.size(),nullptr,0);if(n>0){std::wstring w(n,L' ');MultiByteToWideChar(CP_UTF8,0,s.data(),s.size(),w.data(),n);int k=WideCharToMultiByte(CP_UTF8,0,w.data(),n,nullptr,0,nullptr,nullptr);string clean(k,' ');WideCharToMultiByte(CP_UTF8,0,w.data(),n,clean.data(),k,nullptr,nullptr);s=clean;}
#endif
 return trim(s);}
struct PageInfo{string title,description,status;};
inline string lowerMarkup(string s){for(char&c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline std::map<string,string> htmlAttrs(const string&tag){std::map<string,string>result;size_t i=0;while(i<tag.size()){while(i<tag.size()&&!(std::isalpha((unsigned char)tag[i])||tag[i]=='_'))i++;size_t a=i;while(i<tag.size()&&(std::isalnum((unsigned char)tag[i])||tag[i]=='-'||tag[i]=='_'||tag[i]==':'))i++;string key=lowerMarkup(tag.substr(a,i-a));while(i<tag.size()&&std::isspace((unsigned char)tag[i]))i++;if(i==tag.size())break;if(tag[i]!='='){i++;continue;}i++;while(i<tag.size()&&std::isspace((unsigned char)tag[i]))i++;if(i==tag.size())break;char quote=tag[i];if(quote=='\''||quote=='"'){a=++i;auto end=tag.find(quote,i);if(end==string::npos)break;result[key]=tag.substr(a,end-a);i=end+1;}else{a=i;while(i<tag.size()&&!std::isspace((unsigned char)tag[i])&&tag[i]!='>')i++;result[key]=tag.substr(a,i-a);}}return result;}
inline PageInfo parsePage(const string&html){PageInfo p;auto lower=lowerMarkup(html);auto start=lower.find("<title");if(start!=string::npos){auto a=lower.find('>',start),b=lower.find("</title>",a);if(a!=string::npos&&b!=string::npos)p.title=plainText(html.substr(a+1,std::min<size_t>(b-a-1,4096)));}
 size_t pos=0;while((pos=lower.find("<meta",pos))!=string::npos){auto end=lower.find('>',pos);if(end==string::npos)break;if(end-pos<=8192){auto attrs=htmlAttrs(html.substr(pos+5,end-pos-5));if(lowerMarkup(attrs["property"])=="og:title")p.title=plainText(attrs["content"]);if(lowerMarkup(attrs["name"])=="description"||lowerMarkup(attrs["property"])=="og:description")p.description=plainText(attrs["content"]);}pos=end+1;}
 if(hasAny(lowerMarkup(p.title),{"just a moment","access denied","attention required","captcha"})){p.title="";p.description="";p.status="Pagina protegida; consulta indisponivel";}else p.status=p.title.empty()?"Pagina sem metadados legiveis":"Pagina publica consultada";return p;}
#ifdef _WIN32
struct HttpHandle{HINTERNET h;explicit HttpHandle(HINTERNET x):h(x){}~HttpHandle(){if(h)WinHttpCloseHandle(h);}operator HINTERNET()const{return h;}};
inline PageInfo fetchSource(const Source&s){
 HttpHandle session(WinHttpOpen(L"SAMUKXLL-Organizer/1.1 (public mod metadata)",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));if(!session.h)return {"","","Sem conexao HTTP"};WinHttpSetTimeouts(session,2500,2500,2500,2500);
 HttpHandle connection(WinHttpConnect(session,L"www.nexusmods.com",INTERNET_DEFAULT_HTTPS_PORT,0));if(!connection.h)return {"","","Falha de conexao"};string route="/"+s.game+"/mods/"+s.id;std::wstring path(route.begin(),route.end());
 HttpHandle request(WinHttpOpenRequest(connection,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));if(!request.h)return {"","","Falha na consulta"};DWORD disable=WINHTTP_DISABLE_REDIRECTS;WinHttpSetOption(request,WINHTTP_OPTION_DISABLE_FEATURE,&disable,sizeof(disable));
 if(!WinHttpSendRequest(request,L"Accept: text/html\r\nAccept-Encoding: identity\r\n",(DWORD)-1L,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(request,nullptr))return {"","","Consulta indisponivel (rede ou tempo limite)"};
 DWORD status=0,sz=sizeof(status);if(!WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&sz,WINHTTP_NO_HEADER_INDEX))return {"","","Resposta HTTP invalida"};
 if(status!=200)return {"","","HTTP "+std::to_string(status)+"; sem leitura da pagina"};
 string html;char buffer[16384];auto start=std::chrono::steady_clock::now();while(html.size()<512*1024){DWORD got=0;if(!WinHttpReadData(request,buffer,sizeof(buffer),&got))return {"","","Falha durante leitura da pagina"};if(!got)break;html.append(buffer,got);if(std::chrono::steady_clock::now()-start>std::chrono::seconds(8))break;}return parsePage(html);
}
#endif
inline void consultSources(Plan&p,const fs::path&root,const fs::path&cache){
#ifdef _WIN32
 struct Job{Source source;vector<string>mods;PageInfo info;bool cached=false;};vector<Job>jobs;std::map<string,size_t>keys;int missing=0;
 for(auto&m:p.p.mods){Source s;try{auto meta=nativePath(root/fs::u8path(m.name)/L"meta.ini");if(fs::is_regular_file(meta)&&fs::file_size(meta)<1024*1024)s=sourceFromMeta(readFile(meta));}catch(...){ }
 if(!s.valid()){missing++;continue;}auto key=s.game+"-"+s.id;auto it=keys.find(key);if(it==keys.end()){keys[key]=jobs.size();jobs.push_back({s,{m.name},{},false});}else jobs[it->second].mods.push_back(m.name);}
 std::error_code ec;fs::create_directories(cache,ec);for(auto&j:jobs)try{auto file=cache/fs::u8path(j.source.game+"-"+j.source.id+".txt");if(fs::exists(file)&&fs::file_size(file)<16000&&fs::file_time_type::clock::now()-fs::last_write_time(file)<std::chrono::hours(24)){auto l=lines(readFile(file));if(l.size()==3){j.info={decodeHex(l[0]),decodeHex(l[1]),decodeHex(l[2])};j.cached=true;}}}catch(...){ }
 std::atomic<size_t>next{0};std::atomic<int>requests{0};std::atomic<bool>blocked{false};auto start=std::chrono::steady_clock::now();vector<std::thread>workers;
 for(int worker=0;worker<3;worker++)workers.emplace_back([&](){for(;;){size_t i=next.fetch_add(1);if(i>=jobs.size())break;auto&j=jobs[i];if(j.cached)continue;if(blocked||std::chrono::steady_clock::now()-start>std::chrono::seconds(30)||requests.fetch_add(1)>=40){j.info.status="Consulta adiada (limite de tempo/acesso); analise local mantida";continue;}try{j.info=fetchSource(j.source);if(j.info.status.find("HTTP 403")!=string::npos||j.info.status.find("HTTP 429")!=string::npos)blocked=true;if(!j.info.title.empty())try{writeFile(cache/fs::u8path(j.source.game+"-"+j.source.id+".txt"),encodeHex(j.info.title)+"\n"+encodeHex(j.info.description.empty()?" ":j.info.description)+"\n"+encodeHex(j.info.status)+"\n");}catch(...){ }}catch(...){j.info.status="Consulta indisponivel; analise local mantida";}}});
 for(auto&t:workers)t.join();p.online.push_back("Pesquisa de origem no Nexus: "+std::to_string(jobs.size())+" paginas identificadas; "+std::to_string(missing)+" mods sem identificacao suficiente. A consulta online nao altera prioridades.");
 for(auto&j:jobs){string row=j.source.url()+"\r\n"+(j.cached?"Cache de ate 24h. ":"")+j.info.status;for(auto&name:j.mods)row+="\r\nMod: "+name;if(!j.info.title.empty())row+="\r\nTitulo: "+j.info.title;if(!j.info.description.empty())row+="\r\nResumo da pagina: "+j.info.description;p.online.push_back(row);}
#else
 p.online.push_back("Consulta online disponivel no executavel Windows.");
#endif
}
