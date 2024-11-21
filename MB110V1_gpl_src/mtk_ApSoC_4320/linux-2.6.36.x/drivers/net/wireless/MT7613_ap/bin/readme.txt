############################################################################## Method ##############################################################################
mt7613 更新fw和patch：
（1）将要更新的.bin文件放到 mtk_ApSoC_4320\linux-2.6.36.x\drivers\net\wireless\bin\mt7663\rebb\
（2）在编译目录 mtk_ApSoC_4320\linux-2.6.36.x\drivers\net\wireless\MT7613\embedded下执行
		make build_tools CHIPSET=mt7663，编译成对应的.h文件（embedded/include/mcu）
（3）回到source目录下编译软件

可以通过mt_wifi/embedded/include/mcu中.h的日期去确认是否更新成功。
开机启动后通过iwpriv rai0 show driverinfo也会有fw的build date可以确认。
 

############################################################################## History ##############################################################################

fw version:		190726
patch version:	190726

新fw主要更新如下问题：
1. [MT7663]tpc_duty 無法config by user define
2. [MT7663]T-put(tx duty) cannot recover to 100% when thermal down.
3. [[Test Mode]For CONNAC Architecture]For CONNAC Architecture]Disble dynmaic clock for fixed 7663 tx hang issue]
4. [MT7663][IBF] iBF phase cal with abnormal behavior at TP-Link project]
5. [MT7663]Thermal compensation offset（0x1BBh）不生效


