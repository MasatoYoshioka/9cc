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

assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 10 '5 * (1 + 1)'
assert 5 '10 * 2 / 4'
assert 4 '9 - 10 / 2'
assert 10 '1 + 3 * 3'
assert 2 '8 / 2 / 2'
assert 8 '2 * 2 * 2'
assert 1 ' 1 /1 '
assert 4 '8 / 2'
assert 49 '7 * 7'
assert 3 '1 + 1 + 1'
assert 21 '5 + 20 - 4'
assert 0 0
assert 43 43
echo OK
