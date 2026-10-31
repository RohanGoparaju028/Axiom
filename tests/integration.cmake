cmake_minimum_required(VERSION 3.16)
string(RANDOM LENGTH 10 ALPHABET abcdef0123456789 nonce)
set(root "${WORK}/run ${nonce}")
file(MAKE_DIRECTORY "${root}")
if(WIN32)
  set(suffix .exe)
endif()
function(command cwd expected)
  execute_process(COMMAND "${COMPILER}" ${ARGN} WORKING_DIRECTORY "${cwd}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 60)
  if(NOT status EQUAL expected)
    message(FATAL_ERROR "${ARGN}: expected ${expected}, got ${status}\n${out}\n${err}")
  endif()
  string(REPLACE "\r\n" "\n" out "${out}")
  set(output "${out}" PARENT_SCOPE)
  set(diagnostic "${err}" PARENT_SCOPE)
endfunction()
function(program name source expected)
  file(WRITE "${root}/${name}.ax" "${source}")
  command("${root}" 0 run "${name}.ax")
  if(NOT output STREQUAL expected)
    message(FATAL_ERROR "${name}: expected [${expected}], got [${output}]")
  endif()
endfunction()
function(reject name source reason)
  file(WRITE "${root}/${name}.ax" "${source}")
  command("${root}" 1 check "${name}.ax")
  if(NOT diagnostic MATCHES "${reason}")
    message(FATAL_ERROR "${name}: expected diagnostic ${reason}, got ${diagnostic}")
  endif()
endfunction()
function(runtime_failure name source reason)
  file(WRITE "${root}/${name}.ax" "${source}")
  command("${root}" 1 run "${name}.ax")
  if(NOT diagnostic MATCHES "Axiom runtime error:.*${reason}")
    message(FATAL_ERROR "${name}: wrong runtime diagnostic ${diagnostic}")
  endif()
endfunction()

command("${root}" 0 new "hello world")
set(project "${root}/hello world")
command("${project}/src" 0 check)
command("${project}/src" 0 run)
if(NOT output STREQUAL "Hello from Axiom!\n42\n")
  message(FATAL_ERROR "project startup failed: ${output}")
endif()
command("${project}" 0 build --debug)
if(NOT EXISTS "${project}/axiom-cxx/debug/hello-world${suffix}" OR NOT EXISTS "${project}/axiom-cxx/release/hello-world${suffix}")
  message(FATAL_ERROR "separate build profiles missing")
endif()
command("${root}" 1 new "hello world")
command("${project}" 1 init)
file(SHA256 "${project}/axiom-cxx/release/hello-world${suffix}" old_binary)
file(WRITE "${project}/src/main.ax" "fn main()->i32{return missing;}")
command("${project}" 1 run)
file(SHA256 "${project}/axiom-cxx/release/hello-world${suffix}" new_binary)
if(NOT old_binary STREQUAL new_binary)
  message(FATAL_ERROR "failed check destroyed the previous binary")
endif()
file(MAKE_DIRECTORY "${root}/existing")
file(WRITE "${root}/existing/.gitignore" "keep-me\n")
command("${root}/existing" 0 init)
file(READ "${root}/existing/.gitignore" ignore)
if(NOT ignore STREQUAL "keep-me\n")
  message(FATAL_ERROR "init overwrote an existing ignore file")
endif()
file(MAKE_DIRECTORY "${root}/conflict/src")
file(WRITE "${root}/conflict/src/main.ax" "existing source")
command("${root}/conflict" 1 init)
if(EXISTS "${root}/conflict/Axiom.toml")
  message(FATAL_ERROR "conflicting init partially created the project")
endif()

command("${root}" 0 new native --cpp)
command("${root}/native" 0 run)
if(NOT output STREQUAL "42\n")
  message(FATAL_ERROR "C++ library integration failed")
endif()
# A failed native compilation also preserves the last successful artifact.
file(SHA256 "${root}/native/axiom-cxx/release/native${suffix}" old_binary)
file(WRITE "${root}/native/cpp/demo.cpp" "invalid C++")
command("${root}/native" 1 run)
file(SHA256 "${root}/native/axiom-cxx/release/native${suffix}" new_binary)
if(NOT old_binary STREQUAL new_binary)
  message(FATAL_ERROR "failed native compilation replaced the old artifact")
endif()
file(WRITE "${root}/native/cpp/demo.cpp" "#include \"demo.hpp\"\nstd::int32_t demo::twice(std::int32_t x){return x*2;}")
file(WRITE "${root}/native/src/main.ax" "cpp \"demo.hpp\";extern cpp fn twice(x:f64)->f64=\"demo::twice\";fn main()->i32{unsafe{println(twice(21.0));}return 0;}")
command("${root}/native" 1 build)
# Existing overloaded C++ library functions use the exact declared signature.
command("${root}" 0 new math-library)
file(APPEND "${root}/math-library/Axiom.toml" "\n[cpp]\nheaders = [\"cmath\"]\n")
file(WRITE "${root}/math-library/src/main.ax" "extern cpp fn root(x:f64)->f64=\"std::sqrt\";fn main()->i32{unsafe{println(root(81.0));}return 0;}")
command("${root}/math-library" 0 run)
if(NOT output STREQUAL "9\n")
  message(FATAL_ERROR "standard C++ library binding failed: ${output}")
endif()
file(WRITE "${root}/exit-code.ax" "fn main()->i32{return 7;}")
command("${root}" 7 run exit-code.ax)

command("${root}" 0 run "${SOURCE}/examples/tour.ax")
if(NOT output STREQUAL "720\nvalue: 42\nfour\n")
  message(FATAL_ERROR "language tour failed: ${output}")
endif()
program(order [=[
fn combine(a:i32,b:i32)->i32{return a*10+b;}
fn main()->i32{
 let x=1; println(combine(x,x=2)); println(x+(x=10));
 let a:Array<i32>=Array<i32>();a.push(3);let copy=a.clone();copy[0]=8;
 println(a[0]);println(copy[0]);let old=a[0]++;println(old);println(a[0]);
 return 0;
}
]=] "12\n12\n3\n8\n3\n4\n")
program(switch-flow [=[
fn select(n:i32)->i32{switch(n){case 1:return 10;default:return 20;}}
fn main()->i32{
 let total=0;
 for(let i=0;i<5;i++){
   switch(i){case 1:continue;case 3:break;default:total+=i;}
 }
 println(total);println(select(1));
 println(match false {true => "yes", false => "no"});
 println(match -1 {-1 => "negative", _ => "other"});
    println(match "x" {"_" => "underscore", _ => "fallback"});
 return 0;
}
]=] "6\n10\nno\nnegative\nfallback\n")
program(test-evaluation [=[
fn sum(n:i32)->i32{let result=0;for(let i=0;i<=n;i++){result+=i;}return result;}
test {let input=10;const computed:i32=sum(input);assert(computed==55);}
test {assert(computed==55);assert(match true {true=>true,false=>false});}
fn main()->i32{println(computed);return 0;}
]=] "55\n")
file(MAKE_DIRECTORY "${root}/modules")
file(WRITE "${root}/modules/math.ax" "test{const base:i32=40;}pub const amount:i32=base;pub fn add(a:i32,b:i32)->i32{return a+b;}priv fn secret()->i32{return 0;}")
program(imports "import modules.math;fn main()->i32{println(math.add(math.amount,2));return 0;}" "42\n")
reject(private-import "import modules.math;fn main()->i32{return math.secret();}" "unknown")
reject(immutable "fn main()->i32{const x=1;x=2;return 0;}" "immutable")
reject(missing-return "fn main()->i32{if(true){return 0;}}" "every path")
reject(switch-return "fn main()->i32{switch(1){case 1:break;return 0;default:return 0;}}" "every path")
reject(non-exhaustive "enum Result{Ok(value:i32),Err(message:string)}fn main()->i32{let r=Result::Ok(1);return match r{Result::Ok(n)=>n};}" "non-exhaustive")
reject(duplicate-pattern "fn main()->i32{return match true {true=>0,true=>1,false=>2};}" "duplicate")
reject(duplicate-number-pattern "fn main()->i32{return match 1i64 {1=>0,1i64=>1,_=>2};}" "duplicate")
reject(test-assertion "test{assert(false);}fn main()->i32{return 0;}" "compile-time assertion failed")
reject(test-io "test{println(1);}fn main()->i32{return 0;}" "I/O")
reject(test-indirect-io "fn effect()->i32{println(1);return 0;}test{const value:i32=effect();}fn main()->i32{return 0;}" "I/O")
reject(test-limit "test{while(true){}}fn main()->i32{return 0;}" "100000 steps")
reject(test-local "fn main()->i32{test{assert(true);}return 0;}" "expected")
reject(meta-outside-test "fn compute()->i32{return 42;}const answer:i32=compute();fn main()->i32{return 0;}" "global test block")
reject(test-return "test{return;}fn main()->i32{return 0;}" "return is not allowed")
reject(foreign-safe "extern cpp fn f(x:i32)->i32=\"demo::f\";fn main()->i32{return f(1);}" "unsafe")
reject(foreign-test "extern cpp fn f(x:i32)->i32=\"demo::f\";test{unsafe{f(1);}}fn main()->i32{return 0;}" "unsafe")
reject(bad-types "fn main()->i32{let x:i32=\"bad\";return 0;}" "expected i32")
reject(no-uninitialized "fn main()->i32{let x:i32;return 0;}" "expected")
runtime_failure(bounds "fn main()->i32{let a=Array<i32>();println(a[0]);return 0;}" "out of bounds")
runtime_failure(overflow "fn main()->i32{let a:i32=2147483647;println(a+1);return 0;}" "overflow")
runtime_failure(cast "fn main()->i32{println(i32(1.5));return 0;}" "fractional")
runtime_failure(cast-boundary "fn main()->i32{println(i64(9223372036854775808.0));return 0;}" "out of range")
reject(test-cast-boundary "test{const value:i64=i64(9223372036854775808.0);}fn main()->i32{return 0;}" "out of range")
runtime_failure(divzero "fn main()->i32{println(1/0);return 0;}" "division by zero")
runtime_failure(divoverflow "fn main()->i32{println(-9223372036854775808i64 / -1);return 0;}" "overflow")

# --check needs no native compiler and executes the same compile-time blocks.
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "AXIOM_CXX=/nonexistent/compiler"
  "${COMPILER}" check "${SOURCE}/examples/tour.ax" RESULT_VARIABLE status ERROR_VARIABLE err TIMEOUT 20)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "checking depended on a native compiler: ${err}")
endif()
command("${root}" 0 emit-cpp "${SOURCE}/examples/tour.ax" -o first.cpp)
command("${root}" 0 emit-cpp "${SOURCE}/examples/tour.ax" -o second.cpp)
file(SHA256 "${root}/first.cpp" first)
file(SHA256 "${root}/second.cpp" second)
if(NOT first STREQUAL second)
  message(FATAL_ERROR "code generation was nondeterministic")
endif()
# Detect misspelled manifest keys instead of silently ignoring them.
file(APPEND "${root}/existing/Axiom.toml" "entyr = \"main.ax\"\n")
command("${root}/existing" 1 check)
if(UNIX)
  command("${root}" 0 new symlinked)
  file(MAKE_DIRECTORY "${root}/outside")
  file(CREATE_LINK "${root}/outside" "${root}/symlinked/axiom-cxx" SYMBOLIC)
  command("${root}/symlinked" 1 build)
endif()

# Owning and weak handles are checked by the frontend and exercised natively.
foreach(example IN ITEMS pointers family collections shared_collections linked_nodes debugging)
  command("${root}" 0 run "${SOURCE}/examples/${example}.ax")
endforeach()
program(ownership [=[
class Box { value:i32; }
struct Pair { first:i32; }
fn expired()->Weak<i32>{let p=Ptr<i32>(1);return weak(p);}
fn main()->i32{
 let box=Box(7);let alias=box;
 assert(strong_count(box)==2);alias.value=8;assert(box.value==8);
 {let typed:Ptr<Box>=Ptr<Box>(box);assert(strong_count(box)==3);}
 let observer=weak(box);alias.reset();box.reset();assert(is_expired(observer));
 let p=Ptr<Pair>(Pair(3));p->first=4;assert((*p).first==4);
 let n=Ptr<i32>(40);let nested=Ptr<Ptr<i32>>(n);**nested+=2;assert(*n==42);
 let nested_weak:Weak<Ptr<i32>>=weak(nested);let restored=upgrade(nested_weak);assert(**restored==42);
 let dead=expired();assert(is_empty(upgrade(dead)));
 let values=Array<bool>();values.push(true);values[0]=false;assert(!values[0]);
 let shared=values;assert(strong_count(values)==2);shared[0]=true;assert(values[0]);
 let weak_values=weak(values);shared.reset();values.reset();assert(is_expired(weak_values));
 let map=HashMap<i64,string>();map.set(1,"one");assert(map[1]=="one");
 println("ownership checks passed");return 0;
}
]=] "ownership checks passed\n")
program(recursive-collections [=[
struct Tree{value:i32;children:Array<Tree>;}
fn main()->i32{
 let parent=Tree(1,Array<Tree>());
 parent.children.push(Tree(2,Array<Tree>()));
 assert(parent.children[0].value==2);
 return 0;
}
]=] "")
reject(weak-type "fn main()->i32{let p=Ptr<i32>(1);let w=Weak<string>(p);return 0;}" "expected Ptr<string>")
reject(weak-dereference "fn main()->i32{let w=Weak<i32>();println(*w);return 0;}" "owning pointer")
reject(const-pointer "fn main()->i32{const p=Ptr<i32>(1);*p=2;return 0;}" "immutable")
reject(hash-key "fn main()->i32{let m=HashMap<Ptr<i32>,i32>();return 0;}" "hash keys")
reject(test-heap "test{let p=Ptr<i32>(1);}fn main()->i32{return 0;}" "unavailable")
runtime_failure(empty-owner "fn main()->i32{let p=Ptr<i32>();println(*p);return 0;}" "empty owning pointer")
runtime_failure(missing-key "fn main()->i32{let m=HashMap<string,i32>();println(m.get(\"missing\"));return 0;}" "key not found")
runtime_failure(empty-stack "fn main()->i32{let s=Stack<i32>();println(s.pop());return 0;}" "empty collection")

# ML expression syntax shares the checked runtime and C++ interoperability.
command("${root}" 0 run "${SOURCE}/examples/ml_dialect.ax")
if(NOT output STREQUAL "answer: 42\n7\n")
  message(FATAL_ERROR "ML/C++ dialect example failed: ${output}")
endif()
program(ml_expressions [=[
// ML expression functions also work as entry points.
fn rec factorial(n: int): int = if n <= 1 then 1 else n * factorial(n - 1)
fn choose(flag: bool): i64 = if flag then 42 else 5000000000i64;
fn scoped(x: int): int = let val y = x + 1 in let val x = y * 2 in x end + x end;
fn speak(): unit = if true then println("hello") else println("wrong");
fn main(): int = let val answer = factorial(5) in
  println(answer); println(scoped(3)); println(choose(true)); speak();
  assert((false andalso (1 / 0 == 0)) == false);
  assert(true orelse (1 / 0 == 0));
  println(case ~1 of ~1 => "negative" | _ => "other"); 0
end;
]=] "120\n11\n42\nhello\nnegative\n")
program(ml_compile_time [=[
fn twice(x: int): int = let val y = x * 2 in if y > 0 then y else 0 end;
test { const result: int = twice(21); }
val greeting = if true then "yes" else "no";
fn main()->i32 { println(result); println(greeting); return 0; }
]=] "42\nyes\n")
program(ml_datatype [=[
datatype Shape = Empty | Point of int * int;
fn sum(s: Shape): int = case s of Shape::Empty => 0 | Shape::Point(x,y) => x+y;
fn main()->i32 { println(sum(Shape::Point(20,22))); println(sum(Shape::Empty)); return 0; }
]=] "42\n0\n")
reject(ml_immutable "fn main()->i32 { val x=1; x=2; return 0; }" "immutable")
reject(ml_scope "fn main(): int = (let val x=42 in x end) + x;" "unknown value")
reject(ml_branch_type "fn main(): int = if true then 1 else false;" "incompatible expression types")
reject(ml_condition_type "fn main(): int = if 1 then 1 else 0;" "expected bool")
reject(ml_nonexhaustive "fn main(): int = case true of true => 0;" "non-exhaustive")
reject(ml_duplicate_variant "datatype T = A | A; fn main(): int = 0;" "duplicate variant")

command("${root}" 0 run "${SOURCE}/examples/axiom.ax")
if(NOT output STREQUAL "answer: 42\n120\n42\n7\n")
  message(FATAL_ERROR "Axiom example failed: ${output}")
endif()
program(axiom_functions [=[
fn apply(value: i32, transform: Fn<i32, i32>) = transform(value);
fn make_adder(offset: i32) -> Fn<i32, i32> = fn(value: i32) => value + offset;
fn double(value: i32) = value * 2;
fn choose(flag: bool) -> i32 { if flag { 42 } else { 0 } }
fn main() -> i32 {
    let offset = 10;
    let add = fn(value: i32) => value + offset;
    offset = 99;
    println(apply(32, add));
    println(make_adder(40)(2));
    println(apply(21, double));
    println(choose(true));
    let sum = { let value = 20; value += 22; value };
    println(sum);
    println(if false { 1 / 0 } else { 42 });
    0
}
]=] "42\n42\n42\n42\n42\n42\n")
program(axiom_loops [=[
fn count() -> i32 {
    let total = 0;
    let limit = 5;
    for i in 0..limit {
        limit = 1;
        if i == 2 { continue; }
        total += i;
    }
    for i in 5..0 { total = 999; }
    for i in 0..100 { if i == 3 { break; } total += 1; }
    while total < 12 { total += 1; }
    total
}
test { const result: i32 = count(); assert(result == 12); }
fn main() -> i32 { println(result); println(count()); 0 }
]=] "12\n12\n")
program(axiom_mutual_recursion [=[
fn rec even(n: i32) -> bool = if n == 0 then true else odd(n - 1);
fn rec odd(n: i32) -> bool = if n == 0 then false else even(n - 1);
fn main() -> i32 { println(even(10)); 0 }
]=] "true\n")
program(axiom_native_values [=[
cpp "vector";
cpp "algorithm";
cpp type IntVector = "std::vector<std::int32_t>";
fn main() -> i32 {
    let value = unsafe { cpp::std::max<i32>(20, 42) };
    println(value);
    unsafe {
        let values = IntVector();
        values.push_back::<unit>(42);
        let copy = values;
        values.clear::<unit>();
        println(copy.at::<i32>(0));
        println(values.size::<i64>());
        let greeting = cpp::std::string<string>("Axiom");
        println(greeting.substr::<string>(0, 4));
    }
    0
}
]=] "42\n42\n0\nAxio\n")
reject(axiom_rec_required "fn loop(n:i32)->i32 = loop(n); fn main()->i32 { 0 }" "fn rec")
reject(axiom_mutual_rec_required "fn rec a()->i32 = b(); fn b()->i32 = a(); fn main()->i32 { 0 }" "fn rec")
reject(axiom_recursive_inference "fn rec loop(n:i32) = loop(n); fn main()->i32 { 0 }" "explicit result type")
reject(axiom_closure_capture "fn main()->i32 { let x=1; let f=fn(y:i32)=>{x+=y; x}; 0 }" "immutable")
reject(axiom_range_type "fn main()->i32 { for i in 0..true {} 0 }" "range bounds")
reject(axiom_range_binding "fn main()->i32 { for i in 0..2 { i=10; } 0 }" "immutable")
reject(axiom_native_unsafe "fn main()->i32 = cpp::std::abs<i32>(1);" "unsafe")
reject(axiom_native_test "test { const x:i32 = unsafe { cpp::std::abs<i32>(1) }; }" "unsafe")
reject(axiom_value_block_return "fn main()->i32 { let x={return 1; 0}; 0 }" "return cannot leave")
reject(axiom_old_fun "fun main():int = 0;" "expected def, fn")
reject(axiom_bad_main "fn main(x:i32)->i32 { x }" "entry must be")
file(WRITE "${root}/no-main.ax" "fn start()->i32 { 0 }")
command("${root}" 1 build "no-main.ax")
if(NOT diagnostic MATCHES "needs fn main")
  message(FATAL_ERROR "missing main was not diagnosed: ${diagnostic}")
endif()

file(MAKE_DIRECTORY "${root}/cpp")
file(WRITE "${root}/cpp/callbacks.hpp" [=[
#pragma once
#include <cstdint>
#include <functional>
inline std::int32_t apply_twice(std::function<std::int32_t(std::int32_t)> f, std::int32_t value) {
    return f(f(value));
}
inline std::function<std::int32_t(std::int32_t)> native_adder(std::int32_t offset) {
    return [offset](std::int32_t value) { return value + offset; };
}
]=])
program(axiom_callbacks [=[
cpp "callbacks.hpp";
fn main()->i32 {
    unsafe {
        println(cpp::apply_twice<i32>(fn(value:i32)=>value+1, 40));
        let add = cpp::native_adder<Fn<i32,i32>>(40);
        println(add(2));
    }
    let value = { if false { 0 } else if true { 42 } else { 1 } };
    println(value);
    0
}
]=] "42\n42\n42\n")
file(WRITE "${root}/native_types.ax" "cpp \"vector\"; pub cpp type Values = \"std::vector<std::int32_t>\";")
program(axiom_native_import [=[
import native_types;
fn main()->i32 {
    unsafe { let values=native_types.Values(); values.push_back::<unit>(42); println(values.at::<i32>(0)); }
    0
}
]=] "42\n")
file(WRITE "${root}/native_const.ax" "cpp \"vector\"; cpp type V=\"std::vector<std::int32_t>\"; fn main()->i32 { unsafe { const v=V(); v.push_back::<unit>(1); } 0 }")
command("${root}" 1 build "native_const.ax")
file(WRITE "${root}/native_missing.ax" "fn main()->i32 { unsafe { cpp::not_a_real_function<i32>(); } 0 }")
command("${root}" 1 build "native_missing.ax")

# main uses the same inferred expression/block declarations as any other function.
program(uniform_main [=[
fn answer() = { let number = 40; number += 2; number };
fn main() = { println(answer()); 0 };
]=] "42\n")
program(uniform_main_expression "fn zero() = 0; fn main() = zero();" "")
reject(inferred_main_wrong_result "fn main() = true;" "entry must be")

# Typed native methods prefer ordinary calls and can still select result-typed templates.
file(WRITE "${root}/cpp/template_method.hpp" "struct NativeBox { template<class T> T get() const { return T(42); } };\n")
program(native_template_method "cpp \"template_method.hpp\"; cpp type NativeBox = \"NativeBox\"; def main := unsafe { let box := NativeBox(); println(box.get::<i32>()); };" "42\n")
