; RUN: llvm-as < %s | llc -march=x86 -mattr=+sse,-sse2
; PR2484

define <4 x float> @f4523(<4 x float> %a,<4 x float> %b) nounwind {
entry:
%shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 4,i32
5,i32 2,i32 3>
ret <4 x float> %shuffle
}
