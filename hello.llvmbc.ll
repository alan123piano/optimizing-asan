; ModuleID = 'hello.llvmbc'
source_filename = "hello.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = internal constant { [3 x i8], [29 x i8] } { [3 x i8] c"%d\00", [29 x i8] zeroinitializer }, align 32
@.str.1 = internal constant { [9 x i8], [23 x i8] } { [9 x i8] c"p[%d]=%d\00", [23 x i8] zeroinitializer }, align 32
@___asan_gen_ = private constant [8 x i8] c"<stdin>\00", align 1
@___asan_gen_.1 = private unnamed_addr constant [5 x i8] c".str\00", align 1
@___asan_gen_.2 = private unnamed_addr constant [7 x i8] c".str.1\00", align 1
@llvm.compiler.used = appending global [2 x ptr] [ptr @.str, ptr @.str.1], section "llvm.metadata"
@0 = internal global [2 x { i64, i64, i64, i64, i64, i64, i64, i64 }] [{ i64, i64, i64, i64, i64, i64, i64, i64 } { i64 ptrtoint (ptr @.str to i64), i64 3, i64 32, i64 ptrtoint (ptr @___asan_gen_.1 to i64), i64 ptrtoint (ptr @___asan_gen_ to i64), i64 0, i64 0, i64 -1 }, { i64, i64, i64, i64, i64, i64, i64, i64 } { i64 ptrtoint (ptr @.str.1 to i64), i64 9, i64 32, i64 ptrtoint (ptr @___asan_gen_.2 to i64), i64 ptrtoint (ptr @___asan_gen_ to i64), i64 0, i64 0, i64 -1 }]
@llvm.used = appending global [2 x ptr] [ptr @asan.module_ctor, ptr @asan.module_dtor], section "llvm.metadata"
@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 1, ptr @asan.module_ctor, ptr null }]
@llvm.global_dtors = appending global [1 x { i32, ptr, ptr }] [{ i32, ptr, ptr } { i32 1, ptr @asan.module_dtor, ptr null }]

; Function Attrs: mustprogress nofree nounwind sanitize_address uwtable
define dso_local noundef i32 @_Z7set_ptrPiS_b(ptr nocapture noundef readonly %yabo, ptr nocapture noundef readonly %qabo, i1 noundef zeroext %b) local_unnamed_addr #0 {
entry:
  %yabo.qabo = select i1 %b, ptr %yabo, ptr %qabo
  %0 = ptrtoint ptr %yabo.qabo to i64
  %1 = lshr i64 %0, 3
  %2 = add i64 %1, 2147450880
  %3 = inttoptr i64 %2 to ptr
  %4 = load i8, ptr %3, align 1
  %5 = icmp ne i8 %4, 0
  br i1 %5, label %6, label %12, !prof !5

6:                                                ; preds = %entry
  %7 = and i64 %0, 7
  %8 = add i64 %7, 3
  %9 = trunc i64 %8 to i8
  %10 = icmp sge i8 %9, %4
  br i1 %10, label %11, label %12

11:                                               ; preds = %6
  call void @__asan_report_load4(i64 %0) #5
  unreachable

12:                                               ; preds = %6, %entry
  %13 = load i32, ptr %yabo.qabo, align 4, !tbaa !6
  %call1 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str, i32 noundef %13)
  %14 = ptrtoint ptr %yabo.qabo to i64
  %15 = lshr i64 %14, 3
  %16 = add i64 %15, 2147450880
  %17 = inttoptr i64 %16 to ptr
  %18 = load i8, ptr %17, align 1
  %19 = icmp ne i8 %18, 0
  br i1 %19, label %20, label %26, !prof !5

20:                                               ; preds = %12
  %21 = and i64 %14, 7
  %22 = add i64 %21, 3
  %23 = trunc i64 %22 to i8
  %24 = icmp sge i8 %23, %18
  br i1 %24, label %25, label %26

25:                                               ; preds = %20
  call void @__asan_report_load4(i64 %14) #5
  unreachable

26:                                               ; preds = %20, %12
  %27 = load i32, ptr %yabo.qabo, align 4, !tbaa !6
  ret i32 %27
}

; Function Attrs: nofree nounwind sanitize_address
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #1

; Function Attrs: mustprogress nofree norecurse nounwind sanitize_address uwtable
define dso_local noundef i32 @main() local_unnamed_addr #2 {
entry:
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body
  ret i32 0

for.body:                                         ; preds = %entry, %for.body
  %i.08 = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %call = tail call i32 (ptr, ...) @printf(ptr noundef nonnull @.str.1, i32 noundef %i.08, i32 noundef %i.08)
  %inc = add nuw nsw i32 %i.08, 1
  %exitcond.not = icmp eq i32 %inc, 4
  br i1 %exitcond.not, label %for.cond.cleanup, label %for.body, !llvm.loop !10
}

declare void @__asan_report_load_n(i64, i64)

declare void @__asan_loadN(i64, i64)

declare void @__asan_report_load1(i64)

declare void @__asan_load1(i64)

declare void @__asan_report_load2(i64)

declare void @__asan_load2(i64)

declare void @__asan_report_load4(i64)

declare void @__asan_load4(i64)

declare void @__asan_report_load8(i64)

declare void @__asan_load8(i64)

declare void @__asan_report_load16(i64)

declare void @__asan_load16(i64)

declare void @__asan_report_store_n(i64, i64)

declare void @__asan_storeN(i64, i64)

declare void @__asan_report_store1(i64)

declare void @__asan_store1(i64)

declare void @__asan_report_store2(i64)

declare void @__asan_store2(i64)

declare void @__asan_report_store4(i64)

declare void @__asan_store4(i64)

declare void @__asan_report_store8(i64)

declare void @__asan_store8(i64)

declare void @__asan_report_store16(i64)

declare void @__asan_store16(i64)

declare void @__asan_report_exp_load_n(i64, i64, i32)

declare void @__asan_exp_loadN(i64, i64, i32)

declare void @__asan_report_exp_load1(i64, i32)

declare void @__asan_exp_load1(i64, i32)

declare void @__asan_report_exp_load2(i64, i32)

declare void @__asan_exp_load2(i64, i32)

declare void @__asan_report_exp_load4(i64, i32)

declare void @__asan_exp_load4(i64, i32)

declare void @__asan_report_exp_load8(i64, i32)

declare void @__asan_exp_load8(i64, i32)

declare void @__asan_report_exp_load16(i64, i32)

declare void @__asan_exp_load16(i64, i32)

declare void @__asan_report_exp_store_n(i64, i64, i32)

declare void @__asan_exp_storeN(i64, i64, i32)

declare void @__asan_report_exp_store1(i64, i32)

declare void @__asan_exp_store1(i64, i32)

declare void @__asan_report_exp_store2(i64, i32)

declare void @__asan_exp_store2(i64, i32)

declare void @__asan_report_exp_store4(i64, i32)

declare void @__asan_exp_store4(i64, i32)

declare void @__asan_report_exp_store8(i64, i32)

declare void @__asan_exp_store8(i64, i32)

declare void @__asan_report_exp_store16(i64, i32)

declare void @__asan_exp_store16(i64, i32)

declare ptr @__asan_memmove(ptr, ptr, i64)

declare ptr @__asan_memcpy(ptr, ptr, i64)

declare ptr @__asan_memset(ptr, i32, i64)

declare void @__asan_handle_no_return()

declare void @__sanitizer_ptr_cmp(i64, i64)

declare void @__sanitizer_ptr_sub(i64, i64)

; Function Attrs: nounwind readnone speculatable willreturn
declare i1 @llvm.amdgcn.is.shared(ptr nocapture) #3

; Function Attrs: nounwind readnone speculatable willreturn
declare i1 @llvm.amdgcn.is.private(ptr nocapture) #3

declare void @__asan_before_dynamic_init(i64)

declare void @__asan_after_dynamic_init()

declare void @__asan_register_globals(i64, i64)

declare void @__asan_unregister_globals(i64, i64)

declare void @__asan_register_image_globals(i64)

declare void @__asan_unregister_image_globals(i64)

declare void @__asan_register_elf_globals(i64, i64, i64)

declare void @__asan_unregister_elf_globals(i64, i64, i64)

declare void @__asan_init()

; Function Attrs: nounwind uwtable
define internal void @asan.module_ctor() #4 {
  call void @__asan_init()
  call void @__asan_version_mismatch_check_v8()
  call void @__asan_register_globals(i64 ptrtoint (ptr @0 to i64), i64 2)
  ret void
}

declare void @__asan_version_mismatch_check_v8()

; Function Attrs: nounwind uwtable
define internal void @asan.module_dtor() #4 {
  call void @__asan_unregister_globals(i64 ptrtoint (ptr @0 to i64), i64 2)
  ret void
}

attributes #0 = { mustprogress nofree nounwind sanitize_address uwtable "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree nounwind sanitize_address "frame-pointer"="none" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { mustprogress nofree norecurse nounwind sanitize_address uwtable "frame-pointer"="none" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind readnone speculatable willreturn }
attributes #4 = { nounwind uwtable }
attributes #5 = { nomerge }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version 15.0.6"}
!5 = !{!"branch_weights", i32 1, i32 100000}
!6 = !{!7, !7, i64 0}
!7 = !{!"int", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C++ TBAA"}
!10 = distinct !{!10, !11, !12}
!11 = !{!"llvm.loop.mustprogress"}
!12 = !{!"llvm.loop.unroll.disable"}
