############################################################################## Method ##############################################################################
mt7613 ����fw��patch��
��1����Ҫ���µ�.bin�ļ��ŵ� mtk_ApSoC_4320\linux-2.6.36.x\drivers\net\wireless\bin\mt7663\rebb\
��2���ڱ���Ŀ¼ mtk_ApSoC_4320\linux-2.6.36.x\drivers\net\wireless\MT7613\embedded��ִ��
		make build_tools CHIPSET=mt7663������ɶ�Ӧ��.h�ļ���embedded/include/mcu��
��3���ص�sourceĿ¼�±������

����ͨ��mt_wifi/embedded/include/mcu��.h������ȥȷ���Ƿ���³ɹ���
����������ͨ��iwpriv rai0 show driverinfoҲ����fw��build date����ȷ�ϡ�
 

############################################################################## History ##############################################################################

fw version:		190726
patch version:	190726

��fw��Ҫ�����������⣺
1. [MT7663]tpc_duty �o��config by user define
2. [MT7663]T-put(tx duty) cannot recover to 100% when thermal down.
3. [[Test Mode]For CONNAC Architecture]For CONNAC Architecture]Disble dynmaic clock for fixed 7663 tx hang issue]
4. [MT7663][IBF] iBF phase cal with abnormal behavior at TP-Link project]
5. [MT7663]Thermal compensation offset��0x1BBh������Ч


