; RUN: opt < %s -basicaa -dse -S | FileCheck %s
target datalayout = "E-p:64:64:64-a0:0:8-f32:32:32-f64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-v64:64:64-v128:128:128"

define void @test1(i32* %Q, i32* %P) {
        %DEAD = load i32* %Q
        store i32 %DEAD, i32* %P
        store i32 0, i32* %P
        ret void
; CHECK: @test1
; CHECK-NEXT: store i32 0, i32* %P
; CHECK-NEXT: ret void
}

