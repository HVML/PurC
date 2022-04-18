# test case sample
#
# test_begin_index
# param_begin
# your parameters
# param_end
# your result
# test_end

# variant in test case:
# undefined:;
# null:;
# boolean:true;
# number:3.1415926;
# longint:3;
# ulongint:5;
# longdouble:3.1415926;
# string:"hello world"
# atromstring:"hello world"
# bsequence:"hello world"
# dynamic:;
# native:;
# object:2:"key1";boolean:true;"key2";boolean:false;
# array:2:boolean:true;boolean:false;
# set:2:object:2:"key1";boolean:true;"key2";boolean:false;object:2:"key1";boolean:false;"key2";boolean:true;
# invalid:;

# Notation:
# 1. NO white space is permitted in a line;
# 2. NO BLANK LINE is permitted in a test case;
# 3. One variant must end with ';';
# 4. The contents in dynamic and native type, are all pointers. So the code constructs pointers, do not input anything.

test_begin
param_begin
param_end
invalid:;
test_end

test_begin
param_begin
null:;
param_end
invalid:;
test_end

test_begin
param_begin
undefined:;
boolean:false;
param_end
invalid:;
test_end

test_begin
param_begin
null:;
number:1;
undefined:;
param_end
invalid:;
test_end

test_begin
param_begin
string:"caseless";
number:1;
undefined:;
param_end
boolean:false;
test_end

test_begin
param_begin
string:"caseless";
number:0;
undefined:;
param_end
boolean:false;
test_end

test_begin
param_begin
string:"caseless";
string:"hello world";
string:"HELLO WORLD";
param_end
boolean:true;
test_end

test_begin
param_begin
string:"case";
string:"hello world";
string:"HELLO WORLD";
param_end
boolean:false;
test_end

test_begin
param_begin
string:"case";
string:"hello world";
string:"hello world";
param_end
boolean:true;
test_end

test_begin
param_begin
string:"reg";
string:"he.*";
string:"hello world";
param_end
boolean:true;
test_end

test_begin
param_begin
string:"reg";
string:"he[lbc]lo world";
string:"hello world";
param_end
boolean:true;
test_end

test_begin
param_begin
string:"reg";
string:"he[abc]lo world";
string:"hello world";
param_end
boolean:false;
test_end

test_begin
param_begin
string:"wildcard";
string:"hel*o world";
string:"hello world";
param_end
boolean:true;
test_end

test_begin
param_begin
string:"wildcard";
string:"he?o world";
string:"hello world";
param_end
boolean:false;
test_end



