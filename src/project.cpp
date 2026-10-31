#include "project.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace axiom {
bool within(const fs::path& root,const fs::path& path){auto r=fs::weakly_canonical(root),p=fs::weakly_canonical(path);auto a=r.begin(),b=p.begin();for(;a!=r.end();++a,++b)if(b==p.end()||*a!=*b)return false;return true;}
namespace {
std::string trim(std::string text){auto a=text.find_first_not_of(" \t\r\n"),b=text.find_last_not_of(" \t\r\n");return a==std::string::npos?"":text.substr(a,b-a+1);}
bool valid_name(const std::string& name){return name.size()<=64&&std::regex_match(name,std::regex("[A-Za-z0-9][A-Za-z0-9_-]*"));}
std::string project_name(const fs::path& path){auto name=path.filename().string();for(char& c:name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))c='-';return valid_name(name)?name:"app";}
std::vector<std::string> strings(const std::string& value,bool array,const Token& token){
    std::vector<std::string> out;size_t pos=0;auto skip=[&]{while(pos<value.size()&&std::isspace(static_cast<unsigned char>(value[pos])))++pos;};skip();if(array){if(pos==value.size()||value[pos++]!='[')throw Error("expected an array of quoted strings",token);skip();}
    while(pos<value.size()&&(!array||value[pos]!=']')){
        skip();if(pos>=value.size()||value[pos]!='"')throw Error("expected a quoted string",token);auto begin=pos++;bool closed=false;
        while(pos<value.size()){if(value[pos]=='\\'){pos+=2;continue;}if(value[pos++]=='"'){closed=true;break;}}
        if(!closed)throw Error("unterminated manifest string",token);Token t{value.substr(begin,pos-begin),"string",token.file,token.line,token.column};out.push_back(decode_string(t));skip();if(!array)break;if(pos<value.size()&&value[pos]==','){++pos;skip();}else break;
    }
    if(array){skip();if(pos==value.size()||value[pos++]!=']')throw Error("expected closing ]",token);}skip();if(pos!=value.size())throw Error("unexpected manifest text",token);if(!array&&out.size()!=1)throw Error("expected a quoted string",token);return out;
}
fs::path project_path(const Project& p,const std::string& name,bool directory){fs::path relative(name);if(name.empty()||name.find('\0')!=std::string::npos||relative.is_absolute()||relative.has_root_name())throw Error("manifest paths must be relative to the project");auto path=p.root/relative;if(!within(p.root,path))throw Error("manifest path escapes the project: "+name);if(directory?!fs::is_directory(path):!fs::is_regular_file(path))throw Error("manifest path does not exist: "+name);return fs::canonical(path);}
}
Project open_project(const fs::path& start,bool required,bool dependencies){
    auto root=fs::absolute(start);while(!fs::exists(root/"Axiom.toml")){auto parent=root.parent_path();if(parent==root){if(required)throw Error("no Axiom.toml found; use axiomc new NAME or axiomc init");return {};}root=parent;}
    Project p;p.root=fs::canonical(root);p.entry="src/main.ax";std::istringstream input(read_file(root/"Axiom.toml"));std::string text,section;int line=0;bool schema=false;std::set<std::string> seen;
    while(std::getline(input,text)){++line;bool quoted=false,escaped=false;for(size_t i=0;i<text.size();++i){char c=text[i];if(escaped){escaped=false;continue;}if(c=='\\'&&quoted){escaped=true;continue;}if(c=='"')quoted=!quoted;if(c=='#'&&!quoted){text.resize(i);break;}}text=trim(text);if(text.empty())continue;Token tok{text,"manifest",(root/"Axiom.toml").string(),line,1};
        if(text.front()=='['&&text.back()==']'){section=text.substr(1,text.size()-2);if(section!="project"&&section!="cpp"&&section!="dependencies")throw Error("unknown manifest section",tok);continue;}
        auto equal=text.find('=');if(equal==std::string::npos)throw Error("expected key = value",tok);auto key=trim(text.substr(0,equal)),value=trim(text.substr(equal+1));if(!seen.insert(section+'.'+key).second)throw Error("duplicate manifest key",tok);
        if(section=="project"){
            if(key=="schema"){if(value!="1")throw Error("supported manifest schema is 1",tok);schema=true;}
            else if(key=="name"){p.name=strings(value,false,tok)[0];if(!valid_name(p.name))throw Error("project name must use 1-64 letters, digits, underscores, or hyphens",tok);}
            else if(key=="entry"){p.entry=strings(value,false,tok)[0];}
            else if(key=="cxx_dir"){p.cxx_dir=strings(value,false,tok)[0];if(p.cxx_dir.empty())throw Error("cxx_dir must not be empty",tok);}
            else throw Error("unknown project key '"+key+"'",tok);
        }else if(section=="dependencies"){
            Unit check;check.file=root/"Axiom.toml";parse(check,"import "+key+";");
            if(key.find('.')!=std::string::npos||key=="std")throw Error("dependency name must be a single non-standard module identifier",tok);
            p.dependencies[key]=strings(value,false,tok)[0];
        }else if(section=="cpp"){
            auto items=strings(value,true,tok);
            for(const auto& item:items){
                if(key=="headers"){if(!valid_cpp_header(item))throw Error("invalid C++ header name",tok);p.options.headers.push_back(item);}
                else if(key=="libraries"){if(!std::regex_match(item,std::regex("[A-Za-z0-9_][A-Za-z0-9_.+-]*")))throw Error("invalid library name",tok);p.options.libraries.push_back(item);}
                else if(key=="sources"){auto path=project_path(p,item,false);auto ext=path.extension().string();if(ext!=".cpp"&&ext!=".cc"&&ext!=".cxx"&&ext!=".c++"&&ext!=".c")throw Error("native sources must use .cpp, .cc, .cxx, .c++, or .c",tok);p.options.sources.push_back(path);}
                else if(key=="include_dirs")p.options.include_dirs.push_back(project_path(p,item,true));
                else if(key=="library_dirs")p.options.library_dirs.push_back(project_path(p,item,true));
                else throw Error("unknown C++ key '"+key+"'",tok);
            }
            if(items.empty()&&key!="headers"&&key!="sources"&&key!="include_dirs"&&key!="library_dirs"&&key!="libraries")throw Error("unknown C++ key '"+key+"'",tok);
        }else throw Error("keys must appear under [project] or [cpp]",tok);
    }
    if(!schema||p.name.empty())throw Error("Axiom.toml requires [project] schema = 1 and name");
    if(dependencies)p.entry=project_path(p,p.entry.string(),false);
    else {auto candidate=p.root/p.entry;if(!within(p.root,candidate))throw Error("entry escapes project");p.entry=fs::weakly_canonical(candidate);}
    if(p.entry.extension()!=".ax"&&p.entry.extension()!=".ar"&&p.entry.extension()!=".fe")throw Error("entry must use .ax (or legacy .ar/.fe)");if(dependencies)prepare_project(p);return p;
}
namespace { void collect_dependencies(Project&,std::set<std::string>&); }
void prepare_project(Project& p){
    std::set<std::string> active;collect_dependencies(p,active);
    if(p.cxx_dir.empty())p.cxx_dir="axiom-cxx";
    p.cxx_dir=fs::absolute(p.root/p.cxx_dir).lexically_normal();
    auto native=p.root/"cpp";
    if(fs::exists(native)){
        if(!fs::is_directory(native)||fs::is_symlink(fs::symlink_status(native)))throw Error("cpp must be a local directory");
        p.options.include_dirs.insert(p.options.include_dirs.begin(),fs::canonical(native));
        for(const auto& item:fs::recursive_directory_iterator(native)){
            if(fs::is_symlink(item.symlink_status()))throw Error("cpp must not contain symbolic links: "+item.path().string());
            if(!item.is_regular_file())continue;
            auto ext=item.path().extension().string();
            if(ext==".cpp"||ext==".cc"||ext==".cxx"||ext==".c++"||ext==".c")p.options.sources.push_back(fs::canonical(item.path()));
        }
    }
    std::sort(p.options.sources.begin(),p.options.sources.end());
    p.options.sources.erase(std::unique(p.options.sources.begin(),p.options.sources.end()),p.options.sources.end());
}
fs::path build_directory(const Project& p,bool debug){
    auto base=fs::absolute(p.root/(p.cxx_dir.empty()?fs::path("axiom-cxx"):p.cxx_dir)).lexically_normal();
    if(base.string().find('\0')!=std::string::npos)throw Error("invalid C++ output directory");
    auto directory=base/(debug?"debug":"release");
    if(fs::is_symlink(fs::symlink_status(base))||fs::is_symlink(fs::symlink_status(directory)))throw Error("C++ output directory cannot be a symbolic link");
    if(fs::weakly_canonical(base)==fs::weakly_canonical(p.root)||within(p.root/"src",base)||within(p.root/"cpp",base))
        throw Error("C++ output directory must be separate from the project root, src, and cpp");
    for(const auto& file:p.options.sources)if(within(base,file))throw Error("C++ output directory contains native source files");
    return directory;
}
void validate_output(const Compiled& program,const Project& p,const fs::path& output){
    if(fs::is_symlink(fs::symlink_status(output)))throw Error("output cannot be a symbolic link");
    if(fs::exists(output)&&(!fs::is_regular_file(output)||fs::hard_link_count(output)>1))throw Error("output must be a regular file without hard-link aliases");
    auto protected_files=program.sources;
    protected_files.insert(protected_files.end(),p.options.sources.begin(),p.options.sources.end());
    protected_files.push_back(p.root/"Axiom.toml");
    for(const auto& file:protected_files){
        if(fs::weakly_canonical(file)==fs::weakly_canonical(output)||(fs::exists(file)&&fs::exists(output)&&fs::equivalent(file,output)))throw Error("output must not overwrite source files");
    }
    if(within(p.root/"cpp",output))throw Error("output must not overwrite files in cpp");
}
void create_project(const fs::path& directory,bool fresh,bool cpp,const fs::path& cxx_dir){
    auto root=fs::weakly_canonical(fs::absolute(directory));if(fresh&&fs::exists(fs::symlink_status(root)))throw Error("destination exists; use axiomc init inside an existing directory");if(fs::exists(root)&&!fs::is_directory(root))throw Error("destination is not a directory");auto name=project_name(root);
    Project config;config.root=root;config.entry=root/"src/main.ax";config.cxx_dir=cxx_dir;
    // Validate the requested layout before creating any files.
    build_directory(config,false);
    std::map<std::string,std::string> files;
    files["Axiom.toml"]="[project]\nschema = 1\nname = "+quote(name)+"\nentry = \"src/main.ax\"\n";
    files["Axiom.toml"]+="cxx_dir = "+quote(cxx_dir.generic_string())+"\n";
    files["cpp/README.md"]="# Native files\n\nCopy library headers and implementations here. In Axiom, use cpp \"name.hpp\"; and direct cpp::namespace::function<Result>(arguments) calls inside unsafe blocks.\nAll .cpp, .cc, .cxx, .c++, and .c files in this directory are compiled recursively.\nDo not copy library examples/tests with their own main function here.\nConfigure additional libraries and include directories in [cpp] in Axiom.toml.\n";
    files["src/main.ax"]="test {\n    const answer: i32 = 6 * 7;\n    assert(answer == 42);\n}\n\ndef main {\n    println(\"Hello from Axiom!\");\n    println(answer);\n    0\n}\n";
    if(cpp){
        files["cpp/demo.hpp"]="#pragma once\n#include <cstdint>\nnamespace demo { std::int32_t twice(std::int32_t value); }\n";
        files["cpp/demo.cpp"]="#include \"demo.hpp\"\nstd::int32_t demo::twice(std::int32_t value) { return value * 2; }\n";
        files["src/main.ax"]="cpp \"demo.hpp\";\n\ndef main {\n    unsafe { println(cpp::demo::twice<i32>(21)); }\n    0\n}\n";
    }
    for(const auto& file:files){auto path=root/file.first;if(!within(root,path)||fs::exists(fs::symlink_status(path)))throw Error("refusing to overwrite "+path.string());auto parent=path.parent_path();if(fs::exists(parent)&&!fs::is_directory(parent))throw Error("project path is not a directory: "+parent.string());}
    for(const auto& file:files){auto path=root/file.first;fs::create_directories(path.parent_path());write_file(path,file.second);}
    if(!fs::exists(fs::symlink_status(root/".gitignore"))){
        std::string ignore=".axiom/\naxiom-cxx/\nbuild/\n";
        auto output=fs::absolute(root/cxx_dir);
        if(cxx_dir!="axiom-cxx"&&within(root,output))ignore+="/"+fs::relative(output,root).generic_string()+"/\n";
        write_file(root/".gitignore",ignore);
    }
    std::cout<<"Created "<<name<<" in "<<root.string()<<"\nRun axiomc run from that directory.\n";
}
fs::path find_cpp_compiler(){
    std::vector<std::string> candidates;if(const char* custom=std::getenv("AXIOM_CXX");custom&&*custom)candidates.push_back(custom);else candidates={"clang++","g++","c++"};
    const char* path=std::getenv("PATH");std::string search=path?path:"";
#ifdef _WIN32
    char separator=';';
#else
    char separator=':';
#endif
    for(auto candidate:candidates){
#ifdef _WIN32
        if(fs::path(candidate).extension().empty())candidate+=".exe";
#endif
        if(fs::path(candidate).has_parent_path()){if(fs::is_regular_file(candidate))return fs::absolute(candidate);continue;}
        std::istringstream parts(search);std::string part;while(std::getline(parts,part,separator)){auto file=fs::path(part)/candidate;if(!fs::is_regular_file(file))continue;
#ifndef _WIN32
            if(access(file.c_str(),X_OK)!=0)continue;
#endif
            return fs::absolute(file);
        }
    }throw Error("C++17 compiler not found; install Clang/GCC or set AXIOM_CXX to one executable path");
}
int execute_process(const std::vector<std::string>& args){
    if(args.empty())throw Error("empty process command");
#ifdef _WIN32
    std::wstring command;for(const auto& arg:args){if(!command.empty())command+=L' ';command+=L'"';size_t slashes=0;for(wchar_t c:fs::path(arg).wstring()){if(c==L'\\'){++slashes;continue;}command.append(c==L'"'?slashes*2+1:slashes,L'\\');slashes=0;command+=c;}command.append(slashes*2,L'\\');command+=L'"';}
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION process{};
    if(!CreateProcessW(nullptr,command.data(),nullptr,nullptr,TRUE,0,nullptr,nullptr,&startup,&process))throw Error("cannot start process: "+args[0]);WaitForSingleObject(process.hProcess,INFINITE);DWORD status=1;GetExitCodeProcess(process.hProcess,&status);CloseHandle(process.hThread);CloseHandle(process.hProcess);return static_cast<int>(status);
#else
    std::vector<char*> argv;for(const auto& arg:args)argv.push_back(const_cast<char*>(arg.c_str()));argv.push_back(nullptr);auto pid=fork();if(pid<0)throw Error("cannot start process");if(pid==0){execvp(argv[0],argv.data());_exit(127);}int status=0;while(waitpid(pid,&status,0)<0)if(errno!=EINTR)throw Error("cannot wait for process");return WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);
#endif
}
int build_native(const Compiled& program,const Project& project,const fs::path& requested,bool debug){
    auto output=fs::absolute(requested);auto directory=build_directory(project,debug);
    validate_output(program,project,output);
    validate_output(program,project,directory/"program.cpp");
    if(fs::is_symlink(fs::symlink_status(output)))throw Error("output cannot be a symbolic link");
    if(fs::exists(output)&&(!fs::is_regular_file(output)||fs::hard_link_count(output)>1))throw Error("output must be a regular file without hard-link aliases");
    for(const auto& source:program.sources)if(fs::weakly_canonical(source)==fs::weakly_canonical(output)||(fs::exists(output)&&fs::equivalent(source,output)))throw Error("output must not overwrite source files");
    static const std::set<std::string> source_extensions={".ax",".ar",".fe",".cpp",".cc",".cxx",".c++",".c",".h",".hpp"};if(source_extensions.count(output.extension().string()))throw Error("executable output must not use a source-file extension");
    if(!fs::is_directory(output.parent_path()))throw Error("output directory does not exist");fs::create_directories(directory);
    static std::atomic<unsigned> sequence{0};fs::path stage;
    for(int i=0;i<100;++i){auto nonce=std::chrono::steady_clock::now().time_since_epoch().count();stage=directory/("stage-"+std::to_string(nonce)+"-"+std::to_string(sequence++));if(fs::create_directory(stage))break;stage.clear();}if(stage.empty())throw Error("cannot create build staging directory");
    struct Cleanup{fs::path p;~Cleanup(){std::error_code ec;fs::remove_all(p,ec);}}cleanup{stage};
    auto source=stage/"program.cpp",binary=stage/"program";
#ifdef _WIN32
    binary+=".exe";
#endif
    write_file(source,program.cpp);
    auto compiler=find_cpp_compiler().string();
    std::vector<std::string> includes;
    for(const auto& inc:project.options.include_dirs){includes.push_back("-I");includes.push_back(inc.string());}
    std::vector<std::string> objects;
    for(const auto& file:project.options.sources)if(file.extension()==".c"){
        auto object=stage/("native-"+std::to_string(objects.size())+".o");
        std::vector<std::string> command={compiler,"-x","c","-std=c11",debug?"-O0":"-O2","-fno-fast-math"};
        if(debug)command.push_back("-g");
        command.insert(command.end(),includes.begin(),includes.end());
        command.insert(command.end(),{"-c",file.string(),"-o",object.string()});
        int status=execute_process(command);if(status!=0)return status;
        objects.push_back(object.string());
    }
    std::vector<std::string> command={compiler,"-std=c++17",debug?"-O0":"-O2","-fno-fast-math"};
    if(debug)command.push_back("-g");
    command.insert(command.end(),includes.begin(),includes.end());
    command.push_back(source.string());
    for(const auto& file:project.options.sources)if(file.extension()!=".c"){
        command.insert(command.end(),{"-x","c++",file.string(),"-x","none"});
    }
    command.insert(command.end(),objects.begin(),objects.end());
    for(const auto& lib:project.options.library_dirs){command.push_back("-L");command.push_back(lib.string());}
    for(const auto& lib:project.options.libraries)command.push_back("-l"+lib);
    command.insert(command.end(),{"-o",binary.string()});
    int status=execute_process(command);if(status!=0)return status;
    auto generated=directory/"program.cpp";if(fs::is_symlink(fs::symlink_status(generated))||(fs::exists(generated)&&fs::hard_link_count(generated)>1))throw Error("generated output has filesystem aliases");fs::copy_file(source,generated,fs::copy_options::overwrite_existing);
#ifdef __APPLE__
    auto staged_symbols=fs::path(binary.string()+".dSYM");
    auto symbol_output=fs::path(output.string()+".dSYM");
    auto previous_symbols=stage/"previous.dSYM";
    bool published_symbols=false,saved_symbols=false;
    if(debug){
        if(!fs::is_directory(staged_symbols)){
            status=execute_process({"dsymutil",binary.string(),"-o",staged_symbols.string()});
            if(status!=0)return status;
        }
        if(fs::exists(fs::symlink_status(symbol_output))){
            if(fs::is_symlink(fs::symlink_status(symbol_output))||!fs::is_regular_file(symbol_output/"Contents/Info.plist"))throw Error("debug symbol output is not a compiler dSYM bundle");
            fs::rename(symbol_output,previous_symbols);saved_symbols=true;
        }
        try{fs::rename(staged_symbols,symbol_output);published_symbols=true;}
        catch(...){if(saved_symbols)fs::rename(previous_symbols,symbol_output);throw;}
    }
#endif
    try{
#ifdef _WIN32
    if(!MoveFileExW(binary.c_str(),output.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw Error("cannot replace output executable");
#else
    fs::rename(binary,output);
#endif
    }catch(...){
#ifdef __APPLE__
        if(published_symbols)fs::rename(symbol_output,staged_symbols);
        if(saved_symbols)fs::rename(previous_symbols,symbol_output);
#endif
        throw;
    }
    return 0;
}
namespace {
void dependency_name(const std::string& name){
    if(!std::regex_match(name,std::regex("[A-Za-z_][A-Za-z0-9_]*"))||name=="std")throw Error("dependency name must be a module identifier other than std");
    Unit unit;parse(unit,"import "+name+";");
}
std::pair<std::string,std::string> pinned(const std::string& spec){
    auto split=spec.rfind('#');
    if(split==std::string::npos||split==0||!std::regex_match(spec.substr(split+1),std::regex("[0-9a-f]{40}([0-9a-f]{24})?")))throw Error("dependencies must use repository#commit; add them with axiom get");
    return {spec.substr(0,split),spec.substr(split+1)};
}
fs::path dependency_path(const fs::path& root,const std::string& name,const std::string& commit){
    dependency_name(name);auto path=root/".axiom"/"deps"/(name+"-"+commit);
    if(!within(root,path))throw Error("dependency cache escapes project");
    return path;
}
fs::path library_entry(const fs::path& root){
    if(fs::is_regular_file(root/"Axiom.toml"))return open_project(root,true,false).entry;
    for(const auto& name:{"src/lib.ax","lib.ax"})if(fs::is_regular_file(root/name))return root/name;
    throw Error("dependency needs Axiom.toml or src/lib.ax or lib.ax: "+root.string());
}
void checked_cache(const fs::path& path,const std::string& commit){
    if(!fs::is_regular_file(path/".git"/"HEAD")||trim(read_file(path/".git"/"HEAD"))!=commit)throw Error("dependency missing or at wrong revision; run axiom get: "+path.string());
}
std::string repository(std::string value){
    if(value.empty()||value[0]=='-'||value.find_first_of("\n\r#")!=std::string::npos||value.find('\0')!=std::string::npos)throw Error("invalid Git repository");
    if(fs::is_directory(value))return fs::canonical(value).string();
    if(value.rfind("https://",0)==0||value.rfind("ssh://",0)==0)return value;
    if(value.rfind("github.com/",0)==0||value.rfind("gitlab.com/",0)==0)return "https://"+value;
    throw Error("use an HTTPS/SSH Git URL or an existing local Git repository");
}
struct Stage {
    fs::path path;
    ~Stage(){std::error_code ec;fs::remove_all(path,ec);}
};
void git(const std::vector<std::string>& arguments){
    std::vector<std::string> command={"git","-c","core.hooksPath=","-c","advice.detachedHead=false"};command.insert(command.end(),arguments.begin(),arguments.end());
    if(execute_process(command)!=0)throw Error("Git dependency operation failed; check the repository, revision, and Git installation");
}
void restore(const fs::path&,const std::map<std::string,std::string>&,std::set<std::string>&);
std::pair<std::string,fs::path> fetch(const fs::path& root,const std::string& name,const std::string& source,const std::string& revision,std::set<std::string>& active){
    if(active.size()>=64)throw Error("dependency graph exceeds 64 levels");
    auto key=source+"#"+revision;if(!active.insert(key).second)throw Error("circular dependency: "+source);
    struct Leave {std::set<std::string>& set;std::string key;~Leave(){set.erase(key);}}leave{active,key};
    auto cache=root/".axiom"/"deps";
    if(!within(root,cache)||fs::is_symlink(root/".axiom")||fs::is_symlink(cache))throw Error("dependency cache must be a local directory");
    fs::create_directories(cache);
    if(std::regex_match(revision,std::regex("[0-9a-f]{40}([0-9a-f]{24})?"))){
        auto existing=dependency_path(root,name,revision);
        if(fs::exists(existing)){
            checked_cache(existing,revision);
            if(!fs::is_regular_file(library_entry(existing)))throw Error("dependency entry is missing: "+existing.string());
            if(fs::exists(existing/"Axiom.toml"))restore(existing,open_project(existing,true,false).dependencies,active);
            return {revision,existing};
        }
    }
    static std::atomic<unsigned> sequence{0};fs::path stage;
    for(int attempt=0;attempt<100;++attempt){auto candidate=cache/("stage-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(sequence++));if(fs::create_directory(candidate)){stage=candidate;break;}}
    if(stage.empty())throw Error("cannot create dependency staging directory");Stage cleanup{stage};auto checkout=stage/"repo";
    git({"clone","--quiet","--no-checkout","--",repository(source),checkout.string()});
    if(revision.empty()||revision[0]=='-'||revision.find_first_of("\n\r")!=std::string::npos)throw Error("invalid Git revision");
    git({"-C",checkout.string(),"checkout","--quiet","--detach",revision,"--"});
    auto commit=trim(read_file(checkout/".git"/"HEAD"));pinned(source+"#"+commit);
    for(auto it=fs::recursive_directory_iterator(checkout);it!=fs::recursive_directory_iterator();++it){
        if(it->path().filename()==".git"){it.disable_recursion_pending();continue;}
        if(it->is_symlink())throw Error("dependency sources must not contain symbolic links");
    }
    auto entry=library_entry(checkout);if(!fs::is_regular_file(entry))throw Error("dependency entry does not exist: "+entry.string());
    if(fs::exists(checkout/"Axiom.toml"))restore(checkout,open_project(checkout,true,false).dependencies,active);
    auto destination=dependency_path(root,name,commit);
    if(!fs::exists(destination))fs::rename(checkout,destination);else checked_cache(destination,commit);
    return {commit,destination};
}
void restore(const fs::path& root,const std::map<std::string,std::string>& dependencies,std::set<std::string>& active){
    for(const auto& dep:dependencies){auto spec=pinned(dep.second);fetch(root,dep.first,spec.first,spec.second,active);}
}
void collect_dependencies(Project& p,std::set<std::string>& active){
    p.options.packages[p.root];
    if(active.size()>=64)throw Error("dependency graph exceeds 64 levels");
    for(const auto& dep:p.dependencies){
        auto spec=pinned(dep.second);auto key=dep.second;
        if(!active.insert(key).second)throw Error("circular dependency: "+dep.first);
        auto root=dependency_path(p.root,dep.first,spec.second);checked_cache(root,spec.second);
        auto entry=library_entry(root);if(!fs::is_regular_file(entry))throw Error("dependency entry missing; run axiom get");
        p.options.packages[p.root][dep.first]=entry;
        Project child;if(fs::exists(root/"Axiom.toml"))child=open_project(root,true,false);else {child.root=root;child.entry=entry;}
        // Collect package mappings in each package's own namespace.
        collect_dependencies(child,active);
        child.dependencies.clear();prepare_project(child);
        p.options.packages.insert(child.options.packages.begin(),child.options.packages.end());
        p.options.headers.insert(p.options.headers.end(),child.options.headers.begin(),child.options.headers.end());
        p.options.sources.insert(p.options.sources.end(),child.options.sources.begin(),child.options.sources.end());
        p.options.include_dirs.insert(p.options.include_dirs.end(),child.options.include_dirs.begin(),child.options.include_dirs.end());
        p.options.library_dirs.insert(p.options.library_dirs.end(),child.options.library_dirs.begin(),child.options.library_dirs.end());
        p.options.libraries.insert(p.options.libraries.end(),child.options.libraries.begin(),child.options.libraries.end());
        active.erase(key);
    }
}
void record_dependency(const Project& p,const std::string& name,const std::string& spec){
    auto manifest=p.root/"Axiom.toml";std::istringstream input(read_file(manifest));std::string output,line;bool section=false,inserted=false,saw=false;
    auto binding=name+" = "+quote(spec)+"\n";
    while(std::getline(input,line)){
        auto clean=trim(line);
        if(!clean.empty()&&clean[0]=='['){
            if(section&&!inserted){output+=binding;inserted=true;}
            section=clean.rfind("[dependencies]",0)==0;saw=saw||section;
        }
        if(section){auto equal=clean.find('=');if(equal!=std::string::npos&&trim(clean.substr(0,equal))==name){output+=binding;inserted=true;continue;}}
        output+=line+'\n';
    }
    if(!inserted){if(!saw)output+="\n[dependencies]\n";output+=binding;}
    // All download and validation work succeeds before changing the manifest.
    auto temporary=p.root/"Axiom.toml.tmp";
    if(fs::exists(fs::symlink_status(temporary)))throw Error("Axiom.toml.tmp already exists");
    write_file(temporary,output);
#ifdef _WIN32
    if(!MoveFileExW(temporary.c_str(),manifest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){fs::remove(temporary);throw Error("cannot update Axiom.toml");}
#else
    fs::rename(temporary,manifest);
#endif
}
}
void init_module(const std::string& requested){
    auto root=fs::current_path();auto manifest=root/"Axiom.toml";
    if(fs::exists(fs::symlink_status(manifest)))throw Error("Axiom.toml already exists");
    auto name=requested.empty()?project_name(root):requested;
    if(!valid_name(name))throw Error("module name must use 1-64 letters, digits, underscores, or hyphens");
    std::string entry="src/main.ax";
    for(const auto& candidate:{"src/lib.ax","lib.ax","src/main.ax","main.ax"})if(fs::is_regular_file(root/candidate)){entry=candidate;break;}
    write_file(manifest,"[project]\nschema = 1\nname = "+quote(name)+"\nentry = "+quote(entry)+"\n\n[dependencies]\n");
    std::cout<<"Created Axiom.toml for "<<name<<"\n";
}
void get_dependency(const std::string& requested,const std::string& requested_name,const std::string& requested_revision){
    auto p=open_project(fs::current_path(),true,false);std::set<std::string> active;
    if(requested.empty()){restore(p.root,p.dependencies,active);std::cout<<"Dependencies ready\n";return;}
    auto source=requested;auto revision=requested_revision.empty()?"HEAD":requested_revision;
    auto at=source.rfind('@');if(at!=std::string::npos&&at>source.find_last_of('/')){if(!requested_revision.empty())throw Error("specify a revision once");revision=source.substr(at+1);source.resize(at);}
    source=repository(source);auto name=requested_name;
    if(name.empty()){name=fs::path(source).filename().string();if(name.size()>4&&name.substr(name.size()-4)==".git")name.resize(name.size()-4);std::replace(name.begin(),name.end(),'-','_');}
    dependency_name(name);auto result=fetch(p.root,name,source,revision,active);
    record_dependency(p,name,source+"#"+result.first);
    std::cout<<"Added "<<name<<" at "<<result.first<<"\n";
}

}
