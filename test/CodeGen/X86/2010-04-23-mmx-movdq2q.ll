; RUN: llc < %s -mtriple=x86_64-apple-darwin -mattr=+mmx,+sse2 | FileCheck %s
; There are no MMX operations here, so we use XMM or i64.

define void @ti8(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to <8 x i8>
        %tmp2 = bitcast double %b to <8 x i8>
        %tmp3 = add <8 x i8> %tmp1, %tmp2
; CHECK:  paddb %xmm1, %xmm0
        store <8 x i8> %tmp3, <8 x i8>* null
        ret void
}

define void @ti16(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to <4 x i16>
        %tmp2 = bitcast double %b to <4 x i16>
        %tmp3 = add <4 x i16> %tmp1, %tmp2
; CHECK:  paddw %xmm1, %xmm0
        store <4 x i16> %tmp3, <4 x i16>* null
        ret void
}

define void @ti32(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to <2 x i32>
        %tmp2 = bitcast double %b to <2 x i32>
        %tmp3 = add <2 x i32> %tmp1, %tmp2
; CHECK:  paddd %xmm1, %xmm0
        store <2 x i32> %tmp3, <2 x i32>* null
        ret void
}

define void @ti64(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to <1 x i64>
        %tmp2 = bitcast double %b to <1 x i64>
        %tmp3 = add <1 x i64> %tmp1, %tmp2
; CHECK:  addq  %rax, %rcx
        store <1 x i64> %tmp3, <1 x i64>* null
        ret void
}

; MMX intrinsics calls get us MMX instructions.

define void @ti8a(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to x86_mmx
; CHECK: movdq2q
        %tmp2 = bitcast double %b to x86_mmx
; CHECK: movdq2q
        %tmp3 = tail call x86_mmx @llvm.x86.mmx.padd.b(x86_mmx %tmp1, x86_mmx %tmp2)
        store x86_mmx %tmp3, x86_mmx* null
        ret void
}

define void @ti16a(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to x86_mmx
; CHECK: movdq2q
        %tmp2 = bitcast double %b to x86_mmx
; CHECK: movdq2q
        %tmp3 = tail call x86_mmx @llvm.x86.mmx.padd.w(x86_mmx %tmp1, x86_mmx %tmp2)
        store x86_mmx %tmp3, x86_mmx* null
        ret void
}

define void @ti32a(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to x86_mmx
; CHECK: movdq2q
        %tmp2 = bitcast double %b to x86_mmx
; CHECK: movdq2q
        %tmp3 = tail call x86_mmx @llvm.x86.mmx.padd.d(x86_mmx %tmp1, x86_mmx %tmp2)
        store x86_mmx %tmp3, x86_mmx* null
        ret void
}

define void @ti64a(double %a, double %b) nounwind {
entry:
        %tmp1 = bitcast double %a to x86_mmx
; CHECK: movdq2q
        %tmp2 = bitcast double %b to x86_mmx
; CHECK: movdq2q
        %tmp3 = tail call x86_mmx @llvm.x86.mmx.padd.q(x86_mmx %tmp1, x86_mmx %tmp2)
        store x86_mmx %tmp3, x86_mmx* null
        ret void
}
 
declare x86_mmx @llvm.x86.mmx.padd.b(x86_mmx, x86_mmx)
declare x86_mmx @llvm.x86.mmx.padd.w(x86_mmx, x86_mmx)
declare x86_mmx @llvm.x86.mmx.padd.d(x86_mmx, x86_mmx)
declare x86_mmx @llvm.x86.mmx.padd.q(x86_mmx, x86_mmx)
