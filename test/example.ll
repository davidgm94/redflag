define i32 @test_nested_loops() {
entry:
  %a = alloca i32, align 4
  store i32 0, i32* %a, align 4
  %b = alloca i32, align 4
  store i32 0, i32* %b, align 4
  %c = alloca i32, align 4
  store i32 0, i32* %c, align 4
  br label %loop_condition

loop_condition:                                   ; preds = %end_loop_block4, %entry
  %b1 = load i32, i32* %b, align 4
  %slt = icmp slt i32 %b1, 10000
  br i1 %slt, label %loop_block, label %end_loop_block

loop_block:                                       ; preds = %loop_condition
  br label %loop_condition2

end_loop_block:                                   ; preds = %loop_condition
  %b15 = load i32, i32* %b, align 4
  ret i32 %b15

loop_condition2:                                  ; preds = %loop_block3, %loop_block
  %a5 = load i32, i32* %a, align 4
  %slt6 = icmp slt i32 %a5, 100
  br i1 %slt6, label %loop_block3, label %end_loop_block4

loop_block3:                                      ; preds = %loop_condition2
  %a7 = load i32, i32* %a, align 4
  %b8 = load i32, i32* %b, align 4
  %mul = mul i32 %a7, %b8
  store i32 %mul, i32* %c, align 4
  %b9 = load i32, i32* %b, align 4
  %c10 = load i32, i32* %c, align 4
  %add = add i32 %b9, %c10
  store i32 %add, i32* %b, align 4
  %a11 = load i32, i32* %a, align 4
  %add12 = add i32 %a11, 1
  store i32 %add12, i32* %a, align 4
  br label %loop_condition2

end_loop_block4:                                  ; preds = %loop_condition2
  %b13 = load i32, i32* %b, align 4
  %add14 = add i32 %b13, 1
  store i32 %add14, i32* %b, align 4
  br label %loop_condition
}
