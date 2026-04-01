; ModuleID = '-'
source_filename = "-"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.margo_print_value_t = type { i32, %union.anon }
%union.anon = type { i64 }
%struct.margo_print_span_t = type { i8, i8, ptr }

@.str = private unnamed_addr constant [5 x i8] c"test\00", align 1
@.str.1 = private unnamed_addr constant [2 x i8] c" \00", align 1
@.str.2 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
@stdout = external global ptr, align 8
@.str.3 = private unnamed_addr constant [5 x i8] c"true\00", align 1
@.str.4 = private unnamed_addr constant [6 x i8] c"false\00", align 1
@.str.5 = private unnamed_addr constant [5 x i8] c"%lld\00", align 1
@.str.6 = private unnamed_addr constant [5 x i8] c"%llu\00", align 1
@.str.7 = private unnamed_addr constant [3 x i8] c"%g\00", align 1
@.str.8 = private unnamed_addr constant [7 x i8] c"(null)\00", align 1
@.str.9 = private unnamed_addr constant [3 x i8] c"%p\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @start(i32 noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca ptr, align 8
  %5 = alloca [1 x %struct.margo_print_value_t], align 8
  %6 = alloca %struct.margo_print_span_t, align 8
  %7 = alloca %struct.margo_print_span_t, align 8
  store i32 %0, ptr %3, align 4
  store ptr %1, ptr %4, align 8
  %8 = call { i32, i64 } @margo_print_value_from_cstr(ptr noundef @.str)
  %9 = getelementptr inbounds { i32, i64 }, ptr %5, i32 0, i32 0
  %10 = extractvalue { i32, i64 } %8, 0
  store i32 %10, ptr %9, align 8
  %11 = getelementptr inbounds { i32, i64 }, ptr %5, i32 0, i32 1
  %12 = extractvalue { i32, i64 } %8, 1
  store i64 %12, ptr %11, align 8
  %13 = getelementptr inbounds [1 x %struct.margo_print_value_t], ptr %5, i64 0, i64 0
  %14 = getelementptr inbounds %struct.margo_print_span_t, ptr %6, i32 0, i32 0
  store i8 0, ptr %14, align 8
  %15 = getelementptr inbounds %struct.margo_print_span_t, ptr %6, i32 0, i32 1
  store i8 0, ptr %15, align 1
  %16 = getelementptr inbounds %struct.margo_print_span_t, ptr %6, i32 0, i32 2
  store ptr null, ptr %16, align 8
  %17 = getelementptr inbounds %struct.margo_print_span_t, ptr %7, i32 0, i32 0
  store i8 0, ptr %17, align 8
  %18 = getelementptr inbounds %struct.margo_print_span_t, ptr %7, i32 0, i32 1
  store i8 0, ptr %18, align 1
  %19 = getelementptr inbounds %struct.margo_print_span_t, ptr %7, i32 0, i32 2
  store ptr null, ptr %19, align 8
  %20 = getelementptr inbounds { i64, ptr }, ptr %6, i32 0, i32 0
  %21 = load i64, ptr %20, align 8
  %22 = getelementptr inbounds { i64, ptr }, ptr %6, i32 0, i32 1
  %23 = load ptr, ptr %22, align 8
  call void @margo_print_emit(ptr noundef %13, i64 noundef 1, i64 %21, ptr %23, i1 noundef zeroext false, ptr noundef byval(%struct.margo_print_span_t) align 8 %7, i1 noundef zeroext false)
  ret i32 0
}

; Function Attrs: noinline nounwind optnone uwtable
define internal void @margo_print_emit(ptr noundef %0, i64 noundef %1, i64 %2, ptr %3, i1 noundef zeroext %4, ptr noundef byval(%struct.margo_print_span_t) align 8 %5, i1 noundef zeroext %6) #0 {
  %8 = alloca %struct.margo_print_span_t, align 8
  %9 = alloca ptr, align 8
  %10 = alloca i64, align 8
  %11 = alloca i8, align 1
  %12 = alloca i8, align 1
  %13 = alloca %struct.margo_print_span_t, align 8
  %14 = alloca %struct.margo_print_span_t, align 8
  %15 = alloca i64, align 8
  %16 = getelementptr inbounds { i64, ptr }, ptr %8, i32 0, i32 0
  store i64 %2, ptr %16, align 8
  %17 = getelementptr inbounds { i64, ptr }, ptr %8, i32 0, i32 1
  store ptr %3, ptr %17, align 8
  store ptr %0, ptr %9, align 8
  store i64 %1, ptr %10, align 8
  %18 = zext i1 %4 to i8
  store i8 %18, ptr %11, align 1
  %19 = zext i1 %6 to i8
  store i8 %19, ptr %12, align 1
  %20 = load i8, ptr %11, align 1
  %21 = trunc i8 %20 to i1
  br i1 %21, label %22, label %23

22:                                               ; preds = %7
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %13, ptr align 8 %8, i64 16, i1 false)
  br label %29

23:                                               ; preds = %7
  %24 = call { i64, ptr } @margo_print_span_from_cstr(ptr noundef @.str.1)
  %25 = getelementptr inbounds { i64, ptr }, ptr %13, i32 0, i32 0
  %26 = extractvalue { i64, ptr } %24, 0
  store i64 %26, ptr %25, align 8
  %27 = getelementptr inbounds { i64, ptr }, ptr %13, i32 0, i32 1
  %28 = extractvalue { i64, ptr } %24, 1
  store ptr %28, ptr %27, align 8
  br label %29

29:                                               ; preds = %23, %22
  %30 = load i8, ptr %12, align 1
  %31 = trunc i8 %30 to i1
  br i1 %31, label %32, label %33

32:                                               ; preds = %29
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %14, ptr align 8 %5, i64 16, i1 false)
  br label %39

33:                                               ; preds = %29
  %34 = call { i64, ptr } @margo_print_span_from_cstr(ptr noundef @.str.2)
  %35 = getelementptr inbounds { i64, ptr }, ptr %14, i32 0, i32 0
  %36 = extractvalue { i64, ptr } %34, 0
  store i64 %36, ptr %35, align 8
  %37 = getelementptr inbounds { i64, ptr }, ptr %14, i32 0, i32 1
  %38 = extractvalue { i64, ptr } %34, 1
  store ptr %38, ptr %37, align 8
  br label %39

39:                                               ; preds = %33, %32
  store i64 0, ptr %15, align 8
  br label %40

40:                                               ; preds = %56, %39
  %41 = load i64, ptr %15, align 8
  %42 = load i64, ptr %10, align 8
  %43 = icmp ult i64 %41, %42
  br i1 %43, label %44, label %59

44:                                               ; preds = %40
  %45 = load i64, ptr %15, align 8
  %46 = icmp ugt i64 %45, 0
  br i1 %46, label %47, label %52

47:                                               ; preds = %44
  %48 = getelementptr inbounds { i64, ptr }, ptr %13, i32 0, i32 0
  %49 = load i64, ptr %48, align 8
  %50 = getelementptr inbounds { i64, ptr }, ptr %13, i32 0, i32 1
  %51 = load ptr, ptr %50, align 8
  call void @margo_print_emit_span(i64 %49, ptr %51)
  br label %52

52:                                               ; preds = %47, %44
  %53 = load ptr, ptr %9, align 8
  %54 = load i64, ptr %15, align 8
  %55 = getelementptr inbounds %struct.margo_print_value_t, ptr %53, i64 %54
  call void @margo_print_emit_value(ptr noundef %55)
  br label %56

56:                                               ; preds = %52
  %57 = load i64, ptr %15, align 8
  %58 = add i64 %57, 1
  store i64 %58, ptr %15, align 8
  br label %40, !llvm.loop !6

59:                                               ; preds = %40
  %60 = getelementptr inbounds { i64, ptr }, ptr %14, i32 0, i32 0
  %61 = load i64, ptr %60, align 8
  %62 = getelementptr inbounds { i64, ptr }, ptr %14, i32 0, i32 1
  %63 = load ptr, ptr %62, align 8
  call void @margo_print_emit_span(i64 %61, ptr %63)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define internal { i32, i64 } @margo_print_value_from_cstr(ptr noundef %0) #0 {
  %2 = alloca %struct.margo_print_value_t, align 8
  %3 = alloca ptr, align 8
  store ptr %0, ptr %3, align 8
  %4 = getelementptr inbounds %struct.margo_print_value_t, ptr %2, i32 0, i32 0
  store i32 5, ptr %4, align 8
  %5 = getelementptr inbounds %struct.margo_print_value_t, ptr %2, i32 0, i32 1
  %6 = load ptr, ptr %3, align 8
  store ptr %6, ptr %5, align 8
  %7 = load { i32, i64 }, ptr %2, align 8
  ret { i32, i64 } %7
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main(i32 noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca ptr, align 8
  store i32 0, ptr %3, align 4
  store i32 %0, ptr %4, align 4
  store ptr %1, ptr %5, align 8
  %6 = load i32, ptr %4, align 4
  %7 = load ptr, ptr %5, align 8
  %8 = call i32 @start(i32 noundef %6, ptr noundef %7)
  ret i32 %8
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #1

; Function Attrs: noinline nounwind optnone uwtable
define internal { i64, ptr } @margo_print_span_from_cstr(ptr noundef %0) #0 {
  %2 = alloca %struct.margo_print_span_t, align 8
  %3 = alloca ptr, align 8
  store ptr %0, ptr %3, align 8
  %4 = getelementptr inbounds %struct.margo_print_span_t, ptr %2, i32 0, i32 0
  store i8 0, ptr %4, align 8
  %5 = getelementptr inbounds %struct.margo_print_span_t, ptr %2, i32 0, i32 1
  store i8 0, ptr %5, align 1
  %6 = getelementptr inbounds %struct.margo_print_span_t, ptr %2, i32 0, i32 2
  %7 = load ptr, ptr %3, align 8
  store ptr %7, ptr %6, align 8
  %8 = load { i64, ptr }, ptr %2, align 8
  ret { i64, ptr } %8
}

; Function Attrs: noinline nounwind optnone uwtable
define internal void @margo_print_emit_span(i64 %0, ptr %1) #0 {
  %3 = alloca %struct.margo_print_span_t, align 8
  %4 = getelementptr inbounds { i64, ptr }, ptr %3, i32 0, i32 0
  store i64 %0, ptr %4, align 8
  %5 = getelementptr inbounds { i64, ptr }, ptr %3, i32 0, i32 1
  store ptr %1, ptr %5, align 8
  %6 = getelementptr inbounds %struct.margo_print_span_t, ptr %3, i32 0, i32 0
  %7 = load i8, ptr %6, align 8
  %8 = trunc i8 %7 to i1
  br i1 %8, label %9, label %15

9:                                                ; preds = %2
  %10 = getelementptr inbounds %struct.margo_print_span_t, ptr %3, i32 0, i32 1
  %11 = load i8, ptr %10, align 1
  %12 = sext i8 %11 to i32
  %13 = load ptr, ptr @stdout, align 8
  %14 = call i32 @fputc(i32 noundef %12, ptr noundef %13)
  br label %25

15:                                               ; preds = %2
  %16 = getelementptr inbounds %struct.margo_print_span_t, ptr %3, i32 0, i32 2
  %17 = load ptr, ptr %16, align 8
  %18 = icmp ne ptr %17, null
  br i1 %18, label %19, label %24

19:                                               ; preds = %15
  %20 = getelementptr inbounds %struct.margo_print_span_t, ptr %3, i32 0, i32 2
  %21 = load ptr, ptr %20, align 8
  %22 = load ptr, ptr @stdout, align 8
  %23 = call i32 @fputs(ptr noundef %21, ptr noundef %22)
  br label %24

24:                                               ; preds = %19, %15
  br label %25

25:                                               ; preds = %24, %9
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define internal void @margo_print_emit_value(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  %4 = icmp ne ptr %3, null
  br i1 %4, label %6, label %5

5:                                                ; preds = %1
  br label %65

6:                                                ; preds = %1
  %7 = load ptr, ptr %2, align 8
  %8 = getelementptr inbounds %struct.margo_print_value_t, ptr %7, i32 0, i32 0
  %9 = load i32, ptr %8, align 8
  switch i32 %9, label %65 [
    i32 0, label %10
    i32 1, label %19
    i32 2, label %25
    i32 3, label %31
    i32 4, label %37
    i32 5, label %44
    i32 6, label %59
  ]

10:                                               ; preds = %6
  %11 = load ptr, ptr %2, align 8
  %12 = getelementptr inbounds %struct.margo_print_value_t, ptr %11, i32 0, i32 1
  %13 = load i8, ptr %12, align 8
  %14 = trunc i8 %13 to i1
  %15 = zext i1 %14 to i64
  %16 = select i1 %14, ptr @.str.3, ptr @.str.4
  %17 = load ptr, ptr @stdout, align 8
  %18 = call i32 @fputs(ptr noundef %16, ptr noundef %17)
  br label %65

19:                                               ; preds = %6
  %20 = load ptr, ptr @stdout, align 8
  %21 = load ptr, ptr %2, align 8
  %22 = getelementptr inbounds %struct.margo_print_value_t, ptr %21, i32 0, i32 1
  %23 = load i64, ptr %22, align 8
  %24 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %20, ptr noundef @.str.5, i64 noundef %23) #4
  br label %65

25:                                               ; preds = %6
  %26 = load ptr, ptr @stdout, align 8
  %27 = load ptr, ptr %2, align 8
  %28 = getelementptr inbounds %struct.margo_print_value_t, ptr %27, i32 0, i32 1
  %29 = load i64, ptr %28, align 8
  %30 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %26, ptr noundef @.str.6, i64 noundef %29) #4
  br label %65

31:                                               ; preds = %6
  %32 = load ptr, ptr @stdout, align 8
  %33 = load ptr, ptr %2, align 8
  %34 = getelementptr inbounds %struct.margo_print_value_t, ptr %33, i32 0, i32 1
  %35 = load double, ptr %34, align 8
  %36 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %32, ptr noundef @.str.7, double noundef %35) #4
  br label %65

37:                                               ; preds = %6
  %38 = load ptr, ptr %2, align 8
  %39 = getelementptr inbounds %struct.margo_print_value_t, ptr %38, i32 0, i32 1
  %40 = load i8, ptr %39, align 8
  %41 = sext i8 %40 to i32
  %42 = load ptr, ptr @stdout, align 8
  %43 = call i32 @fputc(i32 noundef %41, ptr noundef %42)
  br label %65

44:                                               ; preds = %6
  %45 = load ptr, ptr %2, align 8
  %46 = getelementptr inbounds %struct.margo_print_value_t, ptr %45, i32 0, i32 1
  %47 = load ptr, ptr %46, align 8
  %48 = icmp ne ptr %47, null
  br i1 %48, label %49, label %55

49:                                               ; preds = %44
  %50 = load ptr, ptr %2, align 8
  %51 = getelementptr inbounds %struct.margo_print_value_t, ptr %50, i32 0, i32 1
  %52 = load ptr, ptr %51, align 8
  %53 = load ptr, ptr @stdout, align 8
  %54 = call i32 @fputs(ptr noundef %52, ptr noundef %53)
  br label %58

55:                                               ; preds = %44
  %56 = load ptr, ptr @stdout, align 8
  %57 = call i32 @fputs(ptr noundef @.str.8, ptr noundef %56)
  br label %58

58:                                               ; preds = %55, %49
  br label %65

59:                                               ; preds = %6
  %60 = load ptr, ptr @stdout, align 8
  %61 = load ptr, ptr %2, align 8
  %62 = getelementptr inbounds %struct.margo_print_value_t, ptr %61, i32 0, i32 1
  %63 = load ptr, ptr %62, align 8
  %64 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %60, ptr noundef @.str.9, ptr noundef %63) #4
  br label %65

65:                                               ; preds = %5, %6, %59, %58, %37, %31, %25, %19, %10
  ret void
}

declare i32 @fputc(i32 noundef, ptr noundef) #2

declare i32 @fputs(ptr noundef, ptr noundef) #2

; Function Attrs: nounwind
declare i32 @fprintf(ptr noundef, ptr noundef, ...) #3

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (3+b1)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
