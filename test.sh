#!/bin/sh

assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s

    if [ "$?" = 1 ]; then
        echo 'compile error'
        exit 1;
    fi
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $expected"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1;
    fi
}

assert 3 '1 + 1 + 1'
assert 21 '5 + 20 - 4'
assert 0 0
assert 43 43
echo OK
