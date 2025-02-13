LOAD [1] , visit_mode ; 设置visit mode为写入
LOAD ep, C_index

LOAD ep, lowB_index
LOAD ep, raw1

LOAD q , raw2
LOAD ep, visit_index

LOAD [1] , num2
VISIT
DIST

LOAD [1] , alu_mode ; 设定alu模式为"加"
LOAD C_size, num1

LOAD dist_res, C_dist ; TODO: C_dist在LOAD中做一个开关，让数据只能进入W_size起始地址中
LOAD dist_res, lowB_dist

ADD

LOAD [1] , cmp_mode ; 设定比较模式为"等于" ; [x] CSIZE0
LOAD alu_res, C_size

LOAD [0] , cmp2
LOAD C_size, cmp1

LOAD [...], jmp_addr ; 设定跳转后地址为 OUTPUT
CMP

LOAD [1], jmp_mode ; 设置跳转条件: cmp_res=1

JMP ; 如果跳转未执行，才执行本周期剩下的程序11

LOAD [1...1], target

LOAD [0], look_mode ; 设定look_mode为最小
LOAD C_dist, list

LOAD C_dist[10], list
LOOK10

LOAD C_dist[20], list
LOOK10

LOAD C_dist[30], list
LOOK10

LOAD C_dist[40], list
LOOK10

LOAD C_dist[50], list
LOOK10

LOAD [2], cmp_mode ; 设置比较模式为"大于"
LOOK10

LOAD [...], jmp_addr ; 设定跳转后地址为 OUTPUT
LOAD look_res_index, current_node

LOAD look_res_dist , cmp1
LOAD lowB_dist     , cmp2

LOAD [1], jmp_mode ; 设置跳转条件: cmp_res=1
CMP ; 比较C中current的dist是否大于lowB

JMP ; 如果没有跳转，才会执行接下来的指令
; [ ]开始访存
LOAD [140], num1
LOAD current_node, num2

LOAD [2] , alu_mode ; 设定alu模式为"乘"

LOAD [head], num1
MUL

LOAD alu_res, num2
LOAD [1] , alu_mode ; 设定alu模式为"加"

ADD

LOAD [140], dma_offset
LOAD alu_res, dma_addr ; 访问CurrentNode的neignbor

DMA [1] ; 这个指令会将DMA_en拉高以提醒DMA开始工作：去寻找neighbor list和PCA Raw

LOAD CNneighbor_raw, PCA_CNneighbor_raw

PCA

LOAD [0] , i ; i清零，进入找点大循环

LOAD CNneighbor_index, index2adr ; index*128+header=Address

LOAD [32], dma_offset ; 一个完整的点的raw data有128个Byte（offset）
LOAD addr, dma_addr ; 开始访

DMA [1] ; 访问本点的raw data **阻塞**

LOAD [1] , alu_mode ; 设定alu模式为"加"

LOAD [20], num1
LOAD i20, num2

ADD

LOAD alu_res, Const ; [x] PFL

LOAD CNneighbor_index[Const], index2adr ; dma_offset 还是 128

LOAD [32], dma_offset ; 一个完整的点的raw data有128个Byte（offset）
LOAD addr, dma_addr

DMA [0] ; 访问下一个邻居的原始raw data **不用阻塞**

LOAD [0] , cmp_mode ; 设置比较模式为"小于"; [x] CMPi
LOAD i , cmp1

LOAD [16], cmp2

LOAD [...], jmp_addr ; 设定跳转后地址为 CSIZE0
CMP ; 比较是否i<16

LOAD [0] , jmp_mode ; 设置跳转条件: cmp_res=0

JMP

LOAD [0] , Const ; Const先清零
LOAD i , raw_index ; 将 i 加到raw_index上，准备从cache中取raw date
; CNneighbor_index/raw prepared，一直等到DMA出第一个数据后，才执行这一步，每次访问dma_res都会使下个周期dma_res自动更新为下一个
LOAD i20 , Const ; 将 i*20 加到Const上
RAW ; 开始取raw data，确保后面要用的时候已经取完了

LOAD [0] , visit_mode ; 设置visit mode为读出

LOAD CNneighbor_index[Const], visit_index

VISIT

LOAD [1] , cmp_mode ; 设置比较模式为"等于"
LOAD visit_res, cmp1

LOAD [1] , cmp2

LOAD [...], jmp_addr ; 设置跳转后地址为 IPP
CMP ; 比较是否visit过

LOAD [1] , jmp_mode ; 设置跳转条件: cmp_res=1

JMP ; 保证在这儿跳转的时候RAW模块刚好工作完成，处于跳转状态

LOAD [1] , visit_mode ; 设置visit mode为写入

LOAD CNneighbor_index[Const], visit_index

LOAD raw_res, raw1 ; TODO: 接入BUS1024
VISIT

DIST

LOAD [0] , cmp_mode ; 设定比较模式为"小于"
LOAD dist_res, dist1

LOAD [10]  , cmp2 ; ef=10
LOAD W_size, cmp1

LOAD [...], jmp_addr ; 设定跳转后地址为 PFL
CMP

LOAD dist1, cmp1
JMP

LOAD [0] , jmp_mode ; 设置跳转条件: cmp_res=0
LOAD lowB_dist , cmp2 ; ef=10

CMP

JMP

LOAD CNneighbor_index[Const], C_index ; TODO: C_index在LOAD中做一个开关，让数据只能进入W_size起始地址中

LOAD CNneighbor_index[Const], W_index ; TODO: W_index在LOAD中做一个开关，让数据只能进入W_size起始地址中

LOAD [1] , num2
LOAD C_size, num1

LOAD dist1 , C_dist
LOAD dist1 , W_dist

LOAD [1] , alu_mode ; 设定alu模式为"加"

LOAD W_size, num1
ADD

LOAD [1] , num2
LOAD alu_res, C_size

ADD

LOAD [2] , cmp_mode ; 设定比较模式为"大于"
LOAD alu_res, W_size

LOAD [40]  , cmp2
LOAD W_size, cmp1

LOAD [...], jmp_addr ; 设定跳转后地址为 LOOKW
CMP

LOAD [0] , jmp_mode ; 设定跳转模式为: cmp_res=0

LOAD lowB_index, Wrm_index ; 从W中移除此时的lowB_index
JMP

LOAD [1...1], target
WRM ; [x]: 新指令 从W中移除lowB_index，然后自动重排W 要做个新的 “重排模块”

LOAD [1], look_mode ; [x] LOOKW LOOK mode设为最大值
LOAD W_dist, list

LOAD [0] , Const ; 清空Const
LOOK10

LOAD look_res_index, lowB_index
LOAD look_res_index, Const

LOAD W_dist[Const], lowB_dist

LOAD [1] , cmp_res ; 一定跳转

LOAD [...], jmp_addr ; 设置跳转后地址为 CSIZE0

LOAD [1] , jmp_mode ; 设定跳转模式为: cmp_res=1

JMP ; 一定跳转去 CSIZE0

LOAD [1] , num1 ; [x] IPP
LOAD i , num2

LOAD [1] , cmp_res
ADD

LOAD [...], jmp_addr ; 设置跳转后地址为 CMPi
LOAD alu_res, i

LOAD [1] , jmp_mode ; 设定跳转模式为: cmp_res=1

JMP