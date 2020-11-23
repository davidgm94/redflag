define i32 @test_array_elem_assign() {
entry:
  %arr = alloca [2 x i32], align 4
  %arridxaccess = getelementptr inbounds [2 x i32], [2 x i32]* %arr, i32 0, i32 0
  store i32 5, i32* %arridxaccess, align 4
  %arridxaccess1 = getelementptr inbounds [2 x i32], [2 x i32]* %arr, i32 0, i32 0
  %arridxload = load i32, i32* %arridxaccess1, align 4
  ret i32 %arridxload
}
