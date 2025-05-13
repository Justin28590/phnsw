MOV [1] DMAindex ; query is index 1

DMA R

RAW

MOV raw1 raw2 ; raw2 里就一直是query的raw data了。

MOV [2] DMAindex ; ep is index 5373

DMA R

RAW

DIST
MOV DMAindex vst_index

PUSH dist_res DMAindex C

PUSH dist_res DMAindex W

VST W

CMP LE C_size [0] ; [ ] C_size <= 0

JMP [47] ; [x] to the END
MOV [0] wrm_index

RMC wrm_index C
SUB W_size [1]

MOV alu_res acw_index
ACW

CMP GT rmc_dist acw_dist

JMP [47] ; [x] to the END

MOV [0] i

CMP GE i [32] ; [ ] i >= 32

JMP [11] ; [x] to C_size <= 0

MOV i DMAindex

DMA N

NEI

ADD i [1] ; i++

MOV alu_res i

MOV nei_index vst_index
VST R

CMP NE vst_res [0] ; 没有visit过

JMP [18] ; [x] to i >= 32

VST W

SUB W_size [1]

MOV alu_res acw_index
ACW

MOV nei_index DMAindex

DMA R

RAW

DIST

MOV dist_res nei_dist
CMP GE nei_dist acw_dist

JMP [18] ; [x] to i >= 32

CMP GE W_size [40]

JMP [18] ; [x] to i >= 32

PUSH nei_dist nei_index C

PUSH nei_dist nei_index W

CMP LT W_size [40]

JMP [18] ; [x] to i >= 32

SUB W_size [1]

RMW alu_res W

MOV [1] cmp_res

JMP [18] ; [x] to i >= 32

END