#!/bin/sh

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x + y; }
int sub(int x, int y) { return x - y; }

int add6(int a, int b, int c, int d, int e, int f) {
    return a + b + c + d + e + f;
}
EOF

assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s

    if [ "$?" = 1 ]; then
        echo 'compile error'
        exit 1;
    fi
    gcc -static -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $expected"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1;
    fi
}

assert 7  'main() { return add2(3,4); } add2(x,y) { return x+y; }'
assert 1  'main() { return sub2(4,3); } sub2(x,y) { return x-y; }'
assert 55 'main() { return fib(9); } fib(x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'
assert 32 'main() { return ret32(); } ret32() { return 32; }'
assert 8  'main() { return add(3, 5); }'
assert 2  'main() { return sub(5, 3); }'
assert 21 'main() { return add6(1,2,3,4,5,6); }'
assert 3  'main() { return ret3(); }'
assert 5  'main() { return ret5(); }'
assert 3  'main() { 1; {2;} return 3; }'
assert 55 'main() { i=0;j=0;for(i=0;i<=10;i=i+1) j=i+j;return j; }'
assert 55 'main() { i=0;j=0;for(i=0;i<=10;i=i+1){ j=i+j;}return j; }'
assert 3  'main() { for(;;) return 3; return 5; }'
assert 10 'main() { i=0;while(i<10) i=i+1;return i; }'
assert 10 'main() { i=0;while(i<10) { i=i+1; }return i; }'
assert 3  'main() { if (0) return 2; return 3; }'
assert 3  'main() { if (1-1) return 2; return 3; }'
assert 2  'main() { if(1) return 2; return 3 ;}'
assert 2  'main() { if(2-1) return 2; return 3; }'
assert 2  'main() { if(2-1) { return 2;} return 3; }'
assert 2  'main() { if (1) hoge=2; else hoge=1;return hoge; }';
assert 1  'main() { if (0) hoge=2; else hoge=1;return hoge; }';
assert 1  'main() { if (0){ hoge=2; } else {hoge=1;}return hoge; }'
assert 10 'main() { returnhoge = 10; return returnhoge; }'
assert 10 'main() { return 10; }'
assert 10 'main() { return 10;return 20; }'
assert 10 'main() { hoge=5; fuga=2; hoge * fuga; }'
assert 25 'main() { hoge=5; fuga=2; hoge * hoge; }'
#assert 10 'main () {hoge=5; hoge=10;}';
assert 3  'main() { hoge=3; hoge; }'
assert 10 'main() { fuga=5; fuga * 2; }'
assert 7  'main() { fuga=5; fuga + 2; }'
assert 3  'main() { a=3; a; }'
assert 8  'main() { a=3; z=5; a+z; }'
assert 6  'main() { a=b=3; a+b; }'
#assert 2 'main() {1;2;}'
assert 1  'main() { 0<=1; }'
assert 1  'main() { 1<=1; }'
assert 0  'main() { 2<=1; }'
assert 1  'main() { 0<1; }'
assert 0  'main() { 1<1; }'
assert 0  'main() { 2<1; }'
assert 0  'main() { 0>=1; }'
assert 1  'main() { 1>=1; }'
assert 1  'main() { 2>=1; }'
assert 0  'main() { 0>1; }'
assert 0  'main() { 1>1; }'
assert 1  'main() { 2>1; }'
assert 1  'main() { 1==1; }'
assert 0  'main() { 1==0; }'
assert 1  'main() { 1!=0; }'
assert 0  'main() { 1!=1; }'
assert 10 'main() { -10+20; }'
assert 10 'main() { - -10; }'
assert 10 'main() { - - +10; }'
assert 15 'main() { 5*(9-6); }'
assert 4  'main() { (3+5)/2; }'
assert 10 'main() { 5 * (1 + 1); }'
assert 5  'main() { 10 * 2 / 4; }'
assert 4  'main() { 9 - 10 / 2; }'
assert 10 'main() { 1 + 3 * 3; }'
assert 2  'main() { 8 / 2 / 2; }'
assert 8  'main() { 2 * 2 * 2; }'
assert 1  'main() { 1 /1; }'
assert 4  'main() { 8 / 2; }'
assert 49 'main() { 7 * 7; }'
assert 3  'main() { 1 + 1 + 1; }'
assert 21 'main() { 5 + 20 - 4; }'
#assert 0 'main() { 0;}'
#assert 43 'main () {43;}'
echo OK
