; XFAIL: *
; RUN: llvm-as < %s | llc -march=systemz | grep nill | count 1
; RUN: llvm-as < %s | llc -march=systemz | grep nilh | count 1
; RUN: llvm-as < %s | llc -march=systemz | grep nihl | count 1
; RUN: llvm-as < %s | llc -march=systemz | grep nihh | count 1

define i64 @foo1(i64 %a, i64 %b) {
entry:
    %c = and i64 %a, 1
    ret i64 %c
}

define i64 @foo2(i64 %a, i64 %b) {
entry:
    %c = and i64 %a, 131072
    ret i64 %c
}

define i64 @foo3(i64 %a, i64 %b) {
entry:
    %c = and i64 %a, 8589934592
    ret i64 %c
}

define i64 @foo4(i64 %a, i64 %b) {
entry:
    %c = and i64 %a, 562949953421312
    ret i64 %c
}
