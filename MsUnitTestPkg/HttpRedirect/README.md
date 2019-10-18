# Verify Http Redirect to Https functionality

The is a problem with the current Http Stack is when handing a redirect from http to https.  This test does demonstrate the error, but the UEFI system log is needed to examine what is going on.

All of the code in BasicOperations is collected from somewhere in mu_plus DfciPkg\Application\DfciMenu.  There are two versions of some of the code - these duplicate functions have a WORKAROUND suffix.  The WORKAROUND versions work, but the original version is expected to work since the code in HttpDxe has code to "reconfigure" for subsequent operations.

The problem is that the Tcp4Close is not terminating the the session. I do see activity after the Tcp4Close on the socket.

## About

There are 4 test cases in this unit test.  

1. A very simple HttpGet that returns a Hello World page.
2. The failing scenario:
   * The scenario consists of accessing http://mikeytbds3.eastus.cloudapp.azure.com/RedirTest1
   * expects a 302 error, then accesses the location value (https://mikeytbds3.eastus.cloudapp.azure.com/RedirTest2)
3. The same example with per request HttpChild
4. URL w/o a trailing '/' that causes an exception with HEAPGUARD enabled

## HttpRedirectTestApp

| File | What is does |
| --- | --- |
| BasicOperations.c | Code from DfciPkg using the network - slightly modified for this test |
| HttpRedirectTest.c | Code that runs the tests |
| main.py | The web server code for reference. |
| HttpRedirectTestApp.efi | Pre built HttpRedirectTest application |
| UEFI.log | UEFI log showing the errors |


# UEFI.log errors

## Testcase 2 error:

```Text
17:00:12.247 : 827A9F90 Url=https://mikeytbds3.eastus.cloudapp.azure.com/RedirTest2
17:00:12.247 : InstallProtocolInterface: 00CA959F-6CFA-4DB1-95BC-E46C47514390 826F4FC0
17:00:12.248 : InstallProtocolInterface: 1682FE44-BD7A-4407-B7C7-DCA37CA3922D 826F4FE0
17:00:12.248 : InstallProtocolInterface: 41D94CD2-35B6-455A-8258-D4E51334AADD 82490EB0
17:00:12.248 : InstallProtocolInterface: 3AD9DF29-4501-478D-B1F8-7F7FE70E50F3 82494ED0
17:00:12.249 : InstallProtocolInterface: AE3D28CC-E05B-4FA1-A011-7EB55A3F1401 82498EA8
17:00:12.250 : TcpOnAppAbort: connection reset issued by application for TCB 82860EC0
17:00:12.250 : HttpConfigureTcp4 - Access Denied
17:00:12.250 : Http Request failed. Code=Access Denied
17:00:12.250 : Http Get failed.  Status = Access Denied
17:00:12.251 : Bootstrap JSON failed.  Status = Access Denied
17:00:12.251 : MainLogic error. Code=Access Denied
```

## Testcase 4 error:

```Text
17:00:25.017 : 823DAFA8 Url=http://mikeytbds3.eastus.cloudapp.azure.com
17:00:25.017 : InstallProtocolInterface: 41D94CD2-35B6-455A-8258-D4E51334AADD 823B6EB0
17:00:25.018 : InstallProtocolInterface: 3AD9DF29-4501-478D-B1F8-7F7FE70E50F3 823BAED0
17:00:25.018 : InstallProtocolInterface: AE3D28CC-E05B-4FA1-A011-7EB55A3F1401 823C0EA8
17:00:25.018 : InstallProtocolInterface: 09576E91-6D3F-11D2-8E39-00A0C969723B 82338F98
17:00:25.091 : !!!! X64 Exception Type - 0E(#PF - Page-Fault)  CPU Apic ID - 00000000 !!!!
17:00:25.091 : ExceptionData - 0000000000000000  I:0 R:0 U:0 W:0 P:0 PK:0 S:0
17:00:25.091 : RIP  - 00000000870A1A2C, CS  - 0000000000000038, RFLAGS - 0000000000010217
17:00:25.092 : RAX  - 000000000000002F, RCX - 000000008896D220, RDX - 000000008A682D18
17:00:25.092 : RBX  - 00000000823E1000, RSP - 000000008A682E20, RBP - 000000008A682E78
17:00:25.092 : RSI  - 00000000827DCB98, RDI - 0000000000000000
17:00:25.093 : R8   - 000000008BD8B450, R9  - 000000008A682A10, R10 - 0000000086764CC0
17:00:25.093 : R11  - 000000008A682CA0, R12 - 000000008A682F00, R13 - 0000000000000000
17:00:25.094 : R14  - 00000000823C4EF8, R15 - 0000000000000000
17:00:25.094 : DS   - 0000000000000030, ES  - 0000000000000030, FS  - 0000000000000030
17:00:25.094 : GS   - 0000000000000030, SS  - 0000000000000030
17:00:25.095 : CR0  - 0000000080010013, CR2 - 00000000823E1000, CR3 - 000000008A201000
17:00:25.095 : CR4  - 0000000000000668, CR8 - 0000000000000000
17:00:25.095 : DR0  - 0000000000000000, DR1 - 0000000000000000, DR2 - 0000000000000000
17:00:25.096 : DR3  - 0000000000000000, DR6 - 00000000FFFF0FF0, DR7 - 0000000000000400
17:00:25.096 : GDTR - 000000008BE5DE40 0000000000000057, LDTR - 0000000000000000
17:00:25.097 : IDTR - 000000008897C000 0000000000000FFF,   TR - 0000000000000048
17:00:25.097 : FXSAVE_STATE - 000000008BE63450
17:00:25.097 : !!!! Find image HttpDxe.pdb (ImageBase=000000008709D000, EntryPoint=000000008709E324) !!!!
```

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
