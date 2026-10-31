if exists('b:current_syntax') | finish | endif
syn keyword axiomKeyword def fun inductive where with mut val rec type datatype of then end andalso orelse not fn pub priv let const struct class enum test match switch case default if else while for in break continue return import extern cpp unsafe
syn keyword axiomType Fn int real unit i32 i64 f64 bool string void Ptr Weak Array LinkedList Deque Stack Queue HashMap HashSet
syn keyword axiomBoolean true false
syn keyword axiomBuiltin print println read_line assert str length weak upgrade is_empty is_expired strong_count
syn match axiomNumber '\<\d\+\%(\.\d\+\)\?\%([eE][+-]\?\d\+\)\?\%(i32\|i64\|f64\)\?\>'
syn match axiomEscape '\\.' contained
syn region axiomString start='"' skip='\\.' end='"' contains=axiomEscape
syn match axiomComment '//.*$' contains=@Spell
syn region axiomComment start='/\*' end='\*/' contains=@Spell
hi def link axiomKeyword Keyword
hi def link axiomType Type
hi def link axiomBoolean Boolean
hi def link axiomBuiltin Function
hi def link axiomNumber Number
hi def link axiomEscape SpecialChar
hi def link axiomString String
hi def link axiomComment Comment
let b:current_syntax = 'axiom'
