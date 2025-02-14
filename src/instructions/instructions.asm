MOV [1] , visit_mode ; 设置visit mode为写入
MOV ep, C_index

MOV ep, lowB_index
MOV ep, raw1

MOV q , raw2
MOV ep, visit_index

MOV [1] , num2
VISIT
DIST

MOV [1] , alu_mode ; 设定alu模式为"加"
MOV C_size, num1

MOV dist_res, C_dist ; TODO: C_dist在MOV中做一个开关，让数据只能进入W_size起始地址中
MOV dist_res, lowB_dist

ADD

MOV [1] , cmp_mode ; 设定比较模式为"等于" ; [x] CSIZE0
MOV alu_res, C_size

MOV [0] , cmp2
MOV C_size, cmp1

MOV [...], jmp_addr ; 设定跳转后地址为 OUTPUT
CMP

MOV [1], jmp_mode ; 设置跳转条件: cmp_res=1

JMP ; 如果跳转未执行，才执行本周期剩下的程序11

MOV [1...1], target

MOV [0], look_mode ; 设定look_mode为最小
MOV C_dist, list

MOV C_dist[10], list
LOOK10

MOV C_dist[20], list
LOOK10

MOV C_dist[30], list
LOOK10

MOV C_dist[40], list
LOOK10

MOV C_dist[50], list
LOOK10

MOV [2], cmp_mode ; 设置比较模式为"大于"
LOOK10

MOV [...], jmp_addr ; 设定跳转后地址为 OUTPUT
MOV look_res_index, current_node

MOV look_res_dist , cmp1
MOV lowB_dist     , cmp2

MOV [1], jmp_mode ; 设置跳转条件: cmp_res=1
CMP ; 比较C中current的dist是否大于lowB

JMP ; 如果没有跳转，才会执行接下来的指令
; [ ]开始访存
MOV [140], num1
MOV current_node, num2

MOV [2] , alu_mode ; 设定alu模式为"乘"

MOV [head], num1
MUL

MOV alu_res, num2
MOV [1] , alu_mode ; 设定alu模式为"加"

ADD

MOV [140], dma_offset
MOV alu_res, dma_addr ; 访问CurrentNode的neignbor

DMA [1] ; 这个指令会将DMA_en拉高以提醒DMA开始工作：去寻找neighbor list和PCA Raw

MOV CNneighbor_raw, PCA_CNneighbor_raw

PCA

MOV [0] , i ; i清零，进入找点大循环

MOV CNneighbor_index, index2adr ; index*128+header=Address

MOV [32], dma_offset ; 一个完整的点的raw data有128个Byte（offset）
MOV addr, dma_addr ; 开始访

DMA [1] ; 访问本点的raw data **阻塞**

MOV [1] , alu_mode ; 设定alu模式为"加"

MOV [20], num1
MOV i20, num2

ADD

MOV alu_res, Const ; [x] PFL

MOV CNneighbor_index[Const], index2adr ; dma_offset 还是 128

MOV [32], dma_offset ; 一个完整的点的raw data有128个Byte（offset）
MOV addr, dma_addr

DMA [0] ; 访问下一个邻居的原始raw data **不用阻塞**

MOV [0] , cmp_mode ; 设置比较模式为"小于"; [x] CMPi
MOV i , cmp1

MOV [16], cmp2

MOV [...], jmp_addr ; 设定跳转后地址为 CSIZE0
CMP ; 比较是否i<16

MOV [0] , jmp_mode ; 设置跳转条件: cmp_res=0

JMP

MOV [0] , Const ; Const先清零
MOV i , raw_index ; 将 i 加到raw_index上，准备从cache中取raw date
; CNneighbor_index/raw prepared，一直等到DMA出第一个数据后，才执行这一步，每次访问dma_res都会使下个周期dma_res自动更新为下一个
MOV i20 , Const ; 将 i*20 加到Const上
RAW ; 开始取raw data，确保后面要用的时候已经取完了

MOV [0] , visit_mode ; 设置visit mode为读出

MOV CNneighbor_index[Const], visit_index

VISIT

MOV [1] , cmp_mode ; 设置比较模式为"等于"
MOV visit_res, cmp1

MOV [1] , cmp2

MOV [...], jmp_addr ; 设置跳转后地址为 IPP
CMP ; 比较是否visit过

MOV [1] , jmp_mode ; 设置跳转条件: cmp_res=1

JMP ; 保证在这儿跳转的时候RAW模块刚好工作完成，处于跳转状态

MOV [1] , visit_mode ; 设置visit mode为写入

MOV CNneighbor_index[Const], visit_index

MOV raw_res, raw1 ; TODO: 接入BUS1024
VISIT

DIST

MOV [0] , cmp_mode ; 设定比较模式为"小于"
MOV dist_res, dist1

MOV [10]  , cmp2 ; ef=10
MOV W_size, cmp1

MOV [...], jmp_addr ; 设定跳转后地址为 PFL
CMP

MOV dist1, cmp1
JMP

MOV [0] , jmp_mode ; 设置跳转条件: cmp_res=0
MOV lowB_dist , cmp2 ; ef=10

CMP

JMP

MOV CNneighbor_index[Const], C_index ; TODO: C_index在MOV中做一个开关，让数据只能进入W_size起始地址中

MOV CNneighbor_index[Const], W_index ; TODO: W_index在MOV中做一个开关，让数据只能进入W_size起始地址中

MOV [1] , num2
MOV C_size, num1

MOV dist1 , C_dist
MOV dist1 , W_dist

MOV [1] , alu_mode ; 设定alu模式为"加"

MOV W_size, num1
ADD

MOV [1] , num2
MOV alu_res, C_size

ADD

MOV [2] , cmp_mode ; 设定比较模式为"大于"
MOV alu_res, W_size

MOV [40]  , cmp2
MOV W_size, cmp1

MOV [...], jmp_addr ; 设定跳转后地址为 LOOKW
CMP

MOV [0] , jmp_mode ; 设定跳转模式为: cmp_res=0

MOV lowB_index, Wrm_index ; 从W中移除此时的lowB_index
JMP

MOV [1...1], target
WRM ; [x]: 新指令 从W中移除lowB_index，然后自动重排W 要做个新的 “重排模块”

MOV [1], look_mode ; [x] LOOKW LOOK mode设为最大值
MOV W_dist, list

MOV [0] , Const ; 清空Const
LOOK10

MOV look_res_index, lowB_index
MOV look_res_index, Const

MOV W_dist[Const], lowB_dist

MOV [1] , cmp_res ; 一定跳转

MOV [...], jmp_addr ; 设置跳转后地址为 CSIZE0

MOV [1] , jmp_mode ; 设定跳转模式为: cmp_res=1

JMP ; 一定跳转去 CSIZE0

MOV [1] , num1 ; [x] IPP
MOV i , num2

MOV [1] , cmp_res
ADD

MOV [...], jmp_addr ; 设置跳转后地址为 CMPi
MOV alu_res, i

MOV [1] , jmp_mode ; 设定跳转模式为: cmp_res=1

JMP

END