# VxDs. The first Vxd is at 400
#
#400+VXD_ID pascal -register <VxD handler>() <VxD handler>
#
401 pascal -register VXD_VMM() VXD_VMM
405 pascal -register VXD_Timer() VXD_Timer
409 pascal -register VXD_Reboot() VXD_Reboot
410 pascal -register VXD_VDD() VXD_VDD
412 pascal -register VXD_VMD() VXD_VMD
414 pascal -register VXD_Comm() VXD_Comm
#415 pascal -register VXD_Printer() VXD_Printer
423 pascal -register VXD_Shell() VXD_Shell
433 pascal -register VXD_PageFile() VXD_PageFile
438 pascal -register VXD_APM() VXD_APM
439 pascal -register VXD_VXDLoader() VXD_VXDLoader
445 pascal -register VXD_Win32s() VXD_Win32s
451 pascal -register VXD_ConfigMG() VXD_ConfigMG
455 pascal -register VXD_Enable() VXD_Enable
1490 pascal -register VXD_TimerAPI() VXD_TimerAPI
