#include "project.hpp"
#include <iostream>

namespace fs=std::filesystem;
static void usage(){
    std::cout<<"Axiom 0.1.0 — C++17 compiler and integrated project builder\n"
        <<"  axiom mod init [NAME]          create Axiom.toml in the current directory\n"
        <<"  axiom get [REPOSITORY[@REV]] [--name NAME] [--rev REV]\n"
        <<"  axiom new DIRECTORY [--cpp] [--cxx-dir DIRECTORY]\n"
        <<"  axiom init [DIRECTORY] [--cpp] [--cxx-dir DIRECTORY]\n"
        <<"  axiom build [FILE] [--debug] [-o EXECUTABLE] [--cxx-dir DIRECTORY]\n"
        <<"  axiom run [FILE] [--debug] [--cxx-dir DIRECTORY]\n"
        <<"  axiom check [FILE]             check types and global test blocks\n"
        <<"  axiom emit-cpp [FILE] [-o FILE.cpp | --stdout] [--cxx-dir DIRECTORY]\n"
        <<"  axiom FILE [-o PATH]           single-file compile shorthand\n"
        <<"  axiom doctor | --version | --help\n"
        <<"axiomc is a compatible alias for axiom.\n"
        <<"Native files: cpp/; generated output: axiom-cxx/release/ (or debug/).\n"
        <<"new/init persist --cxx-dir in Axiom.toml; build/run resolve it from the current directory.\n";
}
int main(int argc,char** argv){
    try{
        if(argc==1){usage();return 0;}
        std::string command=argv[1];
        if(command=="--help"||command=="-h"){usage();return 0;}
        if(command=="--version"){std::cout<<"Axiom 0.1.0\n";return 0;}
        if(command=="mod"){
            if(argc<3||std::string(argv[2])!="init"||argc>4)throw axiom::Error("usage: axiom mod init [NAME]");
            axiom::init_module(argc==4?argv[3]:"");return 0;
        }
        if(command=="get"){
            std::string repository,name,revision;
            for(int i=2;i<argc;++i){std::string arg=argv[i];
                if(arg=="--name"||arg=="--rev"){auto& value=arg=="--name"?name:revision;if(++i==argc||!value.empty())throw axiom::Error("missing or duplicate get option");value=argv[i];}
                else if(repository.empty()&&!arg.empty()&&arg[0]!='-')repository=arg;
                else throw axiom::Error("usage: axiom get [REPOSITORY[@REV]] [--name NAME] [--rev REV]");
            }
            if(repository.empty()&&(!name.empty()||!revision.empty()))throw axiom::Error("get options require a repository");
            axiom::get_dependency(repository,name,revision);return 0;
        }
        if(command=="doctor"){
            if(argc!=2)throw axiom::Error("usage: axiomc doctor");
            std::cout<<"C++ compiler: "<<axiom::find_cpp_compiler().string()<<'\n';return 0;
        }
        if(command=="new"||command=="init"){
            fs::path path,cxx_dir="axiom-cxx";bool cpp=false,custom=false;
            for(int i=2;i<argc;++i){
                std::string a=argv[i];
                if(a=="--cpp"&&!cpp)cpp=true;
                else if(a=="--cxx-dir"){
                    if(++i==argc||custom||std::string(argv[i]).empty())throw axiom::Error("--cxx-dir requires one directory");
                    cxx_dir=argv[i];custom=true;
                }else if(!a.empty()&&a[0]=='-')throw axiom::Error("unknown project option: "+a);
                else if(path.empty())path=a;
                else throw axiom::Error("specify one project directory");
            }
            if(path.empty()){
                if(command=="new")throw axiom::Error("usage: axiomc new DIRECTORY [--cpp] [--cxx-dir DIRECTORY]");
                path=fs::current_path();
            }
            axiom::create_project(path,command=="new",cpp,cxx_dir);return 0;
        }
        if(command=="--run")command="run";
        if(command=="--check")command="check";
        bool shorthand=command!="build"&&command!="run"&&command!="check"&&command!="emit-cpp";
        fs::path source,output,cxx_dir;bool debug=false,to_stdout=false;
        if(shorthand){
            if(!command.empty()&&command[0]=='-')throw axiom::Error("unknown option: "+command);
            source=command;command="build";
        }
        for(int i=2;i<argc;++i){
            std::string a=argv[i];
            if(a=="--debug"&&!debug)debug=true;
            else if(a=="--stdout"&&!to_stdout)to_stdout=true;
            else if(a=="-o"||a=="--cxx-dir"){
                auto& destination=a=="-o"?output:cxx_dir;
                if(++i==argc||!destination.empty()||std::string(argv[i]).empty())throw axiom::Error(a+" requires one path");
                destination=argv[i];
            }else if(!a.empty()&&a[0]=='-')throw axiom::Error("unknown option: "+a);
            else if(source.empty())source=a;
            else throw axiom::Error("compile one entry file at a time");
        }
        if(command=="check"&&(debug||!output.empty()||!cxx_dir.empty()))throw axiom::Error("check does not accept output options");
        if(command=="run"&&!output.empty())throw axiom::Error("run does not accept -o");
        if(command=="emit-cpp"&&debug)throw axiom::Error("emit-cpp does not accept --debug");
        if(to_stdout&&(command!="emit-cpp"||!output.empty()||!cxx_dir.empty()))throw axiom::Error("--stdout is only for emit-cpp without output paths");
        if(command=="emit-cpp"&&!output.empty()&&!cxx_dir.empty())throw axiom::Error("emit-cpp accepts either -o or --cxx-dir");
        axiom::Project project;
        if(source.empty()){project=axiom::open_project(fs::current_path());source=project.entry;}
        else{
            source=fs::absolute(source);
            if(source.extension()!=".ax"&&source.extension()!=".ar"&&source.extension()!=".fe")throw axiom::Error("source must use .ax (.ar and .fe are legacy extensions)");
            project=axiom::open_project(source.parent_path(),false);
            if(project.root.empty()){
                project.root=fs::canonical(source.parent_path());project.entry=source;project.name=source.stem().string();
                axiom::prepare_project(project);
            }
        }
        if(!cxx_dir.empty())project.cxx_dir=fs::absolute(cxx_dir).lexically_normal();
        auto compiled=axiom::compile(source,project.options,command!="check");
        if(command=="check"){
            std::cout<<"Checked "<<source.filename().string()<<" ("<<compiled.test_blocks<<" compile-time test blocks)\n";return 0;
        }
        if(command=="emit-cpp"){
            if(to_stdout){std::cout<<compiled.cpp;return 0;}
            if(output.empty()){
                auto directory=axiom::build_directory(project,false);
                fs::create_directories(directory);output=directory/"program.cpp";
            }
            axiom::validate_output(compiled,project,output);
            axiom::write_file(output,compiled.cpp);
            std::cout<<"Generated "<<fs::absolute(output).string()<<'\n';return 0;
        }
        if(output.empty()){
            if(shorthand)output=fs::current_path()/source.stem();
            else{
                auto directory=axiom::build_directory(project,debug);
                fs::create_directories(directory);output=directory/project.name;
            }
#ifdef _WIN32
            output+=".exe";
#endif
        }
        int status=axiom::build_native(compiled,project,output,debug);if(status)return status;
        if(command=="run"){
            output=fs::absolute(output);fs::current_path(project.root);
            return axiom::execute_process({output.string()});
        }
        std::cout<<"Built "<<fs::absolute(output).string()<<'\n';return 0;
    }catch(const std::exception& e){std::cerr<<"axiomc: "<<e.what()<<'\n';return 1;}
}
