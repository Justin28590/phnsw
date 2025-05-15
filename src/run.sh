#!/bin/bash

# 初始值和数组
declare -a time_us
i=6

# 循环执行 99 次
for iter in {1..99}
do
    # echo "Iteration $iter: Modifying instructions.asm with i=$i"

    # 定义文件路径
    file_path="/home/miil/phnsw/src/instructions/instructions.asm"

    # 使用 sed 替换第9行的内容为新的值
    sed -i "9c\MOV [$i] DMAindex ; ep is index $i" "$file_path"

    # 执行 make 和 sst 测试
    output=$(make -j && sst ../tests/phnsw-test-001.py)

    # 提取时间并累加
    thistime=$(echo "$output" | grep -E 'Simulation is complete, simulated time: [0-9]+(\.[0-9]+)? (us|ns)' | awk '
    {
        time = $6
        unit = $7

        # 转换 ns 到 us
        if (unit == "ns") {
            time = time / 1000
        }

        print time
    }')

    # 检查命令是否成功执行
    if [ $? -ne 0 ]; then
        echo "Error occurred during iteration $iter. Exiting."
        exit 1
    fi

    time_us+=("$thistime")
    echo "Recorded time: $thistime us"

    # i += 100
    i=$((i + 100))
done

echo "All iterations completed successfully."

# 计算最大时间&算平均时间
max_time=0
sum_time=0
count=${#time_us[@]}

for t in "${time_us[@]}"; do
    # 求和
    sum_time=$(echo "$sum_time + $t" | bc -l)

    # 求最大值
    if (( $(echo "$t > $max_time" | bc -l) )); then
        max_time=$t
    fi
done

avg_time=$(echo "$sum_time / $count" | bc -l)

# 定义输出文件路径
output_file="time_us.csv"

# 写入 CSV 文件
printf "Time (us)\n" > "$output_file"
for t in "${time_us[@]}"; do
    printf "%.3f\n" "$t" >> "$output_file"
done

echo "Time values have been written to $output_file"

# 输出所有记录的时间
echo "Recorded times (in us):"
printf "%.3f " "${time_us[@]}"
printf "\n"
printf "Max time: $max_time us\n"
printf "Average time: %.3f us\n" "$avg_time"
