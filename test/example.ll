@string_lit = private unnamed_addr constant [13 x i8] c"Hello world\0A\00", align 1

declare i32 @puts(i8*)

define void @test_libc_stdio() {
entry:
  %puts = call i32 @puts(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @string_lit, i32 0, i32 0))
  ret void
}
