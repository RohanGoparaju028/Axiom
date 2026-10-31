#include "frontend.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <regex>

namespace axiom {
bool valid_cpp_header(const std::string& name){return std::regex_match(name,std::regex("[A-Za-z0-9_][A-Za-z0-9_./+ -]*"))&&name.find("..") == std::string::npos;}

std::string read_file(const fs::path& path){std::ifstream f(path,std::ios::binary);if(!f)throw Error("cannot read "+path.string());return {std::istreambuf_iterator<char>(f),{}};}
void write_file(const fs::path& path,const std::string& text){std::ofstream f(path,std::ios::binary);f<<text;f.close();if(!f)throw Error("cannot write "+path.string());}
std::string quote(const std::string& text){
    std::string out="\"";for(unsigned char c:text){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c=='\n')out+="\\n";else if(c=='\r')out+="\\r";else if(c=='\t')out+="\\t";else if(c<32||c>=127){out+='\\';out+=char('0'+(c>>6));out+=char('0'+((c>>3)&7));out+=char('0'+(c&7));}else out+=char(c);}return out+'"';
}
std::string decode_string(const Token& t){
    std::string out;for(size_t i=1;i+1<t.text.size();++i){char c=t.text[i];if(c=='\\'){c=t.text[++i];if(c=='n')c='\n';else if(c=='r')c='\r';else if(c=='t')c='\t';else if(c=='0')c='\0';else if(c!='"'&&c!='\\')throw Error("unsupported string escape",t);}out+=c;}return out;
}
std::string spelling(const Type& t){std::string out=t.name;if(!t.args.empty()){out+='<';for(size_t i=0;i<t.args.size();++i){if(i)out+=", ";out+=spelling(t.args[i]);}out+='>';}return out;}
N node(const std::string& kind,const Token& t,const std::string& text){auto n=std::make_shared<Node>();n->kind=kind;n->token=t;n->text=text;return n;}
static bool alpha(char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';}
static bool digit(char c){return c>='0'&&c<='9';}
static std::vector<Token> lex(const std::string& source,const std::string& filename){
    std::vector<Token> out;size_t i=0;int line=1,col=1;
    auto advance=[&]{if(source[i++]=='\n'){++line;col=1;}else ++col;};
    while(i<source.size()){
        if(std::isspace(static_cast<unsigned char>(source[i]))){advance();continue;}
        Token t{"","symbol",filename,line,col};size_t begin=i;
        if(source.compare(i,2,"//")==0){while(i<source.size()&&source[i]!='\n')advance();continue;}
        if(source.compare(i,2,"/*")==0){advance();advance();while(i<source.size()&&source.compare(i,2,"*/")!=0)advance();if(i==source.size())throw Error("unterminated comment",t);advance();advance();continue;}
        if(alpha(source[i])){t.kind="id";while(i<source.size()&&(alpha(source[i])||digit(source[i])))advance();}
        else if(digit(source[i])){
            t.kind="number";while(i<source.size()&&digit(source[i]))advance();
            if(i+1<source.size()&&source[i]=='.'&&digit(source[i+1])){advance();while(i<source.size()&&digit(source[i]))advance();}
            if(i<source.size()&&(source[i]=='e'||source[i]=='E')){advance();if(i<source.size()&&(source[i]=='+'||source[i]=='-'))advance();if(i==source.size()||!digit(source[i]))throw Error("invalid exponent",t);while(i<source.size()&&digit(source[i]))advance();}
            if(source.compare(i,3,"i32")==0||source.compare(i,3,"i64")==0||source.compare(i,3,"f64")==0){advance();advance();advance();}
        }else if(source[i]=='"'){
            t.kind="string";advance();while(i<source.size()&&source[i]!='"'){if(source[i]=='\n'||source[i]=='\r')throw Error("newline in string",t);if(source[i]=='\\'){advance();if(i==source.size())break;}advance();}if(i==source.size())throw Error("unterminated string",t);advance();
        }else{
            static const std::set<std::string> pairs={":=","<>","->","=>","::","==","!=","<=",">=","&&","||","+=","-=","*=","/=","%=","++","--","|>",".."};
            if(pairs.count(source.substr(i,2))){advance();advance();}
            else{if(std::string("{}()[];,:.+-*/%!=<>|~").find(source[i])==std::string::npos)throw Error("unexpected character",t);advance();}
        }
        t.text=source.substr(begin,i-begin);if(t.kind=="string")decode_string(t);out.push_back(t);
    }
    out.push_back({"<eof>","eof",filename,line,col});return out;
}
static const std::set<std::string> reserved={"def","fun","inductive","where","with","mut","int","real","unit","val","rec","type","datatype","of","then","end","andalso","orelse","not","fn","pub","priv","let","const","struct","class","enum","test","match","switch","case","default","if","else","while","for","in","break","continue","return","import","extern","cpp","unsafe","true","false","i32","i64","f64","bool","string","void"};
class Parser {
    Unit& u;std::vector<Token> ts;size_t at=0;int depth=0;
    const Token& token()const{return ts.at(at);}
    bool is(const std::string& t)const{return token().text==t;}
    Token take(const std::string& t=""){auto x=token();if(t==">"&&x.text==">="){ts[at].text="=";++ts[at].column;x.text=">";return x;}if(!t.empty()&&x.text!=t)throw Error("expected '"+t+"', found '"+x.text+"'",x);if(x.kind!="eof")++at;return x;}
    bool accept(const std::string& t){if(!is(t))return false;take();return true;}
    std::string name(){auto t=take();if(t.kind!="id"||reserved.count(t.text)||t.text.rfind("ag_",0)==0||t.text.find("__")!=std::string::npos)throw Error("expected an identifier (ag_ and double underscores are reserved)",t);return t.text;}
    std::string native_name(){auto t=take();if(t.kind!="id")throw Error("expected a native identifier",t);return t.text;}
    Type type(){auto t=take();if(t.kind!="id")throw Error("expected a type",t);Type out{t.text=="int"?"i32":t.text=="real"?"f64":t.text=="unit"?"void":t.text,{}};while(accept("."))out.name+='.'+name();if(accept("<")){out.args.push_back(type());while(accept(","))out.args.push_back(type());take(">");}return out;}
    std::vector<std::pair<std::string,Type>> fields(){std::vector<std::pair<std::string,Type>> out;take("(");if(!is(")"))do{if(is(")"))break;auto n=name();take(":");out.emplace_back(n,type());}while(accept(","));take(")");return out;}
    std::vector<std::pair<std::string,Type>> binders(){
        std::vector<std::pair<std::string,Type>> out;
        while(is("(")){
            take("(");
            while(!is(")")){
                std::vector<std::string> names{name()};
                while(!is(":"))names.push_back(name());
                take(":");auto annotation=type();
                for(const auto& n:names)out.emplace_back(n,annotation);
                if(!accept(","))break;
            }
            take(")");
        }
        return out;
    }
    std::vector<N> arguments(){std::vector<N> out;take("(");if(!is(")"))do{if(is(")"))break;out.push_back(expr());}while(accept(","));take(")");return out;}
    N expr(int min=0){
        if(++depth>128)throw Error("expression nesting exceeds 128",token());struct Guard{int& d;~Guard(){--d;}}guard{depth};
        auto t=take();N n;
        if(t.kind=="number")n=node("number",t,t.text);
        else if(t.kind=="string")n=node("string",t,decode_string(t));
        else if(t.text=="true"||t.text=="false")n=node("boolean",t,t.text);
        else if(t.text=="~"||t.text=="not"||t.text=="-"||t.text=="+"||t.text=="!"||t.text=="*"){
            if((t.text=="-"||t.text=="~")&&token().kind=="number")n=node("number",t,"-"+take().text);
            else{n=node("unary",t,t.text=="~"?"-":t.text=="not"?"!":t.text);n->children={expr(70)};}
        }else if(t.text=="("){n=expr();take(")");}
        else if(t.text=="fn"||t.text=="fun"){
            n=node("lambda",t);n->parameters=t.text=="fun"?binders():fields();take("=>");n->children={expr()};
        }else if(t.text=="cpp"){
            n=node("native_call",t);take("::");n->text=native_name();while(accept("::"))n->text+="::"+native_name();if(accept("<")){n->annotation=type();take(">");}n->children=arguments();
        }else if(t.text=="unsafe"){
            n=node("unsafe_expr",t);n->children={value_block()};
        }else if(t.text=="{"){
            --at;n=value_block();
        }else if(t.text=="if"){
            n=node("conditional",t);n->children.push_back(expr());
            if(accept("then")){n->children.push_back(expr());take("else");n->children.push_back(expr());}
            else{n->children.push_back(value_block());take("else");n->children.push_back(is("if")?expr():value_block());}
        }else if(t.text=="let"){
            n=node("let_expr",t);
            while(!is("in")){if(!is("let")&&!is("val")&&!is("const"))throw Error("expected let binding or in in let expression",token());n->children.push_back(binding(false));accept(";");}
            take("in");auto body=node("sequence",token());body->children.push_back(expr());while(accept(";")){if(is("end"))break;body->children.push_back(expr());}take("end");n->children.push_back(body);
        }else if(t.text=="match"||t.text=="case"){
            bool ml=t.text=="case";n=node("match",t);n->children.push_back(expr());if(ml)take("of");else if(accept("with"))ml=true;else take("{");if(ml)accept("|");while(ml||!accept("}")){
                auto p=take();if((p.text=="-"||p.text=="~")&&token().kind=="number"){p.kind="number";p.text="-"+take().text;}if(p.kind!="id"&&p.kind!="number"&&p.kind!="string")throw Error("expected a pattern",p);auto arm=node("arm",p,p.kind=="string"?decode_string(p):p.text);
                arm->constant=p.kind=="id"&&p.text=="_";
                while(accept("."))arm->text+='.'+name();if(accept("::"))arm->text+="::"+name();
                if(accept("(")){if(!is(")"))do{arm->children.push_back(node("pattern",token(),name()));}while(accept(","));take(")");}
                take("=>");arm->children.push_back(expr());n->children.push_back(arm);if(ml){if(!accept("|"))break;}else if(!is("}"))take(",");
            }
        }else if(t.kind=="id"&&(!reserved.count(t.text)||t.text=="i32"||t.text=="i64"||t.text=="f64"||t.text=="string"||t.text=="bool")){
            n=node("name",t,t.text);static const std::set<std::string> generic={"Array","LinkedList","Deque","Stack","Queue","HashMap","HashSet","Ptr","Weak"};if(generic.count(t.text)&&accept("<")){n->annotation={t.text,{type()}};while(accept(","))n->annotation.args.push_back(type());take(">");}
        }else throw Error("expected an expression",t);
        static const std::map<std::string,int> prec={{"orelse",10},{"andalso",20},{"<>",30},{"=",5},{"+=",5},{"-=",5},{"*=",5},{"/=",5},{"%=",5},{"|>",8},{"||",10},{"&&",20},{"==",30},{"!=",30},{"<",40},{">",40},{"<=",40},{">=",40},{"+",50},{"-",50},{"*",60},{"/",60},{"%",60}};
        int count=0;while(true){if(++count>256)throw Error("expression is too long",token());auto next=token();
            if(min<=80&&is("(")){auto call=node("call",next);call->children={n};auto args=arguments();call->children.insert(call->children.end(),args.begin(),args.end());n=call;}
            else if(min<=80&&is("::")&&at+1<ts.size()&&ts[at+1].text=="<"){
                take("::");take("<");auto call=node("native_method",next,n->text);if(n->kind!="member")throw Error("native method type annotation requires a receiver",next);call->annotation=type();take(">");call->children={n->children[0]};auto args=arguments();call->children.insert(call->children.end(),args.begin(),args.end());n=call;
            }
            else if(min<=80&&(is(".")||is("->")||is("::"))){auto sep=take().text;auto member=node(sep=="::"?"variant":"member",next,sep=="::"?name():native_name());member->children={n};n=member;}
            else if(min<=80&&accept("[")){auto index=node("index",next);index->children={n,expr()};take("]");n=index;}
            else if(min<=80&&(is("++")||is("--"))){auto update=node("update",next,take().text);update->children={n};n=update;}
            else{auto p=prec.find(token().text);if(p==prec.end()||p->second<min)break;auto op=take().text;auto bin=node("binary",next,op=="orelse"?"||":op=="andalso"?"&&":op=="<>"?"!=":op);bin->children={n,expr(p->second+(p->second==5?0:1))};n=bin;}
        }return n;
    }
    void function_tail(N n){
        if(n->kind=="tail")n->kind="return";
        else if(n->kind=="block"&&!n->children.empty())function_tail(n->children.back());
        else if(n->kind=="if"){function_tail(n->children[1]);if(n->children.size()==3)function_tail(n->children[2]);}
        else if(n->kind=="unsafe")function_tail(n->children[0]);
    }
    N block(bool tail=false){auto t=take("{");if(++depth>128)throw Error("block nesting exceeds 128",t);struct Guard{int& d;~Guard(){--d;}}guard{depth};auto n=node("block",t);while(!is("}")){if(is("<eof>"))throw Error("unterminated block",t);auto s=statement();n->children.push_back(s);}take("}");if(tail)function_tail(n);return n;}
    N block_value(N block){
        auto n=node("block_expr",block->token);n->children=block->children;
        if(!n->children.empty()){
            auto last=n->children.back();
            if(last->kind=="tail"){n->children.back()=last->children[0];return n;}
            if(last->kind=="if"&&last->children.size()==3){auto value=node("conditional",last->token);value->children={last->children[0],block_value(last->children[1]),branch_value(last->children[2])};n->children.back()=value;return n;}
        }
        n->children.push_back(node("unit",block->token));return n;
    }
    N branch_value(N branch){
        if(branch->kind=="block")return block_value(branch);
        if(branch->kind=="if"&&branch->children.size()==3){auto n=node("conditional",branch->token);n->children={branch->children[0],block_value(branch->children[1]),branch_value(branch->children[2])};return n;}
        auto n=node("block",branch->token);n->children={branch};return block_value(n);
    }
    N value_block(){return block_value(block());}
    N binding(bool semicolon=true){auto t=take();bool mutable_=t.text=="let"&&accept("mut");auto n=node("binding",t,name());n->constant=t.text=="const"||t.text=="val";if(accept(":"))n->annotation=type();if(accept(":=")){if(t.text=="let")n->constant=!mutable_;}else take("=");n->children={expr()};if(semicolon)take(";");return n;}
    N statement(){auto t=token();
        if(accept(";"))return node("empty",t);
        if(is("{"))return block();
        if(is("let")||is("const")||is("val"))return binding();
        if(accept("return")){auto n=node("return",t);if(!is(";"))n->children={expr()};take(";");return n;}
        if(accept("if")){auto n=node("if",t);n->children={expr()};n->children.push_back(block());if(accept("else"))n->children.push_back(is("if")?statement():block());return n;}
        if(accept("while")){auto n=node("while",t);n->children={expr()};n->children.push_back(block());return n;}
        if(accept("for")){if(!is("(")){
            auto n=node("range_for",t,name());take("in");n->children.push_back(expr());take("..");n->children.push_back(expr());n->children.push_back(block());return n;
        }auto n=node("for",t);take("(");if(is("let")||is("const"))n->children.push_back(binding());else{auto x=node("expression",token());if(!is(";"))x->children={expr()};take(";");n->children.push_back(x);}n->children.push_back(is(";")?node("boolean",token(),"true"):expr());take(";");n->children.push_back(is(")")?node("number",token(),"0"):expr());take(")");n->children.push_back(block());return n;}
        if(accept("switch")){auto n=node("switch",t);take("(");n->children={expr()};take(")");take("{");while(!accept("}")){auto arm=node("case",token());if(accept("case"))arm->children.push_back(expr());else if(accept("default"))arm->constant=true;else throw Error("expected case or default",token());take(":");auto body=node("block",token());while(!is("case")&&!is("default")&&!is("}")){if(is("<eof>"))throw Error("unterminated switch",t);body->children.push_back(statement());}arm->children.push_back(body);n->children.push_back(arm);}return n;}
        if(is("break")||is("continue")){auto n=node(take().text,t);take(";");return n;}
        if(accept("unsafe")){auto n=node("unsafe",t);n->children={block()};return n;}
        auto n=node("expression",t);n->children={expr()};if(is("}"))n->kind="tail";else take(";");return n;
    }
    void header(bool pub=false){
        take("cpp");if(accept("type")){
            auto d=std::make_unique<Decl>();d->token=token();d->kind="native_type";d->public_=pub;d->name=name();take("=");auto target=take();if(target.kind!="string")throw Error("native type requires a quoted C++ type",target);d->foreign=decode_string(target);take(";");add(std::move(d));return;
        }auto t=take();
        if(t.kind!="string")throw Error("cpp requires a quoted header name",t);
        auto name=decode_string(t);
        if(!valid_cpp_header(name))throw Error("invalid C++ header name",t);
        u.headers.push_back(name);take(";");
    }
    void add(std::unique_ptr<Decl> d){if(u.names.count(d->name))throw Error("duplicate declaration '"+d->name+"'",d->token);d->unit=&u;u.names[d->name]=d.get();u.declarations.push_back(std::move(d));}
public:
    Parser(Unit& unit,const std::string& source):u(unit),ts(lex(source,unit.file.string())){}
    void run(){
        while(is("import")||is("cpp")){
            if(is("cpp")){header();continue;}
            take("import");auto n=name();while(accept("."))n+='.'+name();take(";");u.imports.push_back(n);
        }
        while(!is("<eof>")){
            if(accept(";"))continue;
            if(is("cpp")){header();continue;}
            if(accept("test")){auto test=block();u.tests.push_back(test);for(const auto& s:test->children)if(s->kind=="binding"&&s->constant){if(s->annotation.name.empty())throw Error("a constant exported by test needs an explicit type",s->token);auto d=std::make_unique<Decl>();d->kind="constant";d->name=s->text;d->token=s->token;d->result=s->annotation;d->body=s->children[0];d->test_export=true;s->declaration=d.get();add(std::move(d));}continue;}
            bool pub=accept("pub");if(!pub)accept("priv");if(is("cpp")){header(pub);continue;}auto t=token();auto d=std::make_unique<Decl>();d->token=t;d->public_=pub;
            bool foreign=accept("extern");if(foreign)take("cpp");
            if(!foreign&&accept("inductive")){
                d->kind="enum";d->name=name();take("where");take("|");
                do{auto variant=name();if(d->variants.count(variant))throw Error("duplicate variant",t);d->variant_order.push_back(variant);d->variants[variant]=binders();}while(accept("|"));accept(";");
            }else if(!foreign&&accept("datatype")){
                d->kind="enum";d->name=name();take("=");
                do{auto variant=name();if(d->variants.count(variant))throw Error("duplicate variant",t);d->variant_order.push_back(variant);auto& payload=d->variants[variant];
                    if(accept("of")){do{payload.emplace_back("value"+std::to_string(payload.size()),type());}while(accept("*"));}
                }while(accept("|"));accept(";");
            }else if(!foreign&&accept("type")){
                d->kind="struct";d->name=name();take("=");take("{");
                while(!accept("}")){auto field=name();take(":");d->fields.emplace_back(field,type());if(!accept(";")&&!accept(",")&&!is("}"))throw Error("expected ';', ',' or '}' after type field",token());}
                accept(";");
            }else if(is("fn")||(!foreign&&is("def"))){bool lean=take().text=="def";d->kind="function";d->mode=foreign?"foreign":"normal";d->recursive=lean||accept("rec");if(foreign&&d->recursive)throw Error("foreign functions cannot be rec",t);d->name=name();d->parameters=lean?binders():fields();bool annotated=accept("->")||accept(":");d->result=annotated?type():tvoid;
                if(foreign){take("=");auto target=take();if(target.kind!="string")throw Error("C++ binding needs a quoted function name",target);d->foreign=decode_string(target);take(";");}
                else if(lean&&is("{")){
                    if(annotated)d->body=block(true);
                    else{d->infer_result=true;auto ret=node("return",t);ret->children={value_block()};d->body=node("block",t);d->body->children={ret};}
                    accept(";");
                }
                else if(lean?(take(":="),true):accept("=")){
                    d->infer_result=!annotated;auto ret=node("return",t);ret->children={expr()};d->body=node("block",t);d->body->children={ret};accept(";");
                }else d->body=block(true);
            }else if(foreign)throw Error("expected fn after extern cpp",t);
            else if(is("struct")||is("class")||is("enum")){
                d->kind=take().text;d->name=name();take("{");
                while(!accept("}")){auto n=name();if(d->kind!="enum"){take(":");d->fields.emplace_back(n,type());take(";");}else{if(d->variants.count(n))throw Error("duplicate variant",t);d->variant_order.push_back(n);d->variants[n]=is("(")?fields():std::vector<std::pair<std::string,Type>>{};if(!is("}"))take(",");}}
                accept(";");if(d->kind=="enum"&&d->variants.empty())throw Error("enum needs at least one variant",t);
            }else if(!foreign&&is("let")){
                take("let");d->kind="function";d->mode="normal";d->recursive=false;d->name=name();d->parameters=binders();bool annotated=accept(":");d->result=annotated?type():tvoid;take(":=");d->infer_result=!annotated;auto ret=node("return",t);ret->children={expr()};d->body=node("block",t);d->body->children={ret};accept(";");
            }else if(is("const")||is("val")){auto s=binding();d->kind="constant";d->name=s->text;d->result=s->annotation;d->body=s->children[0];}
            else throw Error("expected let, def, fn, inductive, struct, class, enum, datatype, const, val, or global test block",token());
            add(std::move(d));
        }
    }
};
void parse(Unit& u,const std::string& source){Parser(u,source).run();}
}
