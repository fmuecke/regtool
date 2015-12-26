; =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
;
;  Tool to import Registry Keys via commandline
;  
;       Version: 0.95
;  Date created: 2002-09-15
;      Modified: 2003-04-06
;        Author: Florian Muecke (dev[AT]mueckeimnetz.de)
; 
; =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

    .386
    .model flat, stdcall
    option casemap :none   ; case sensitive

; =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

    include c:\masm32\include\windows.inc

    include c:\masm32\include\user32.inc
    include c:\masm32\include\kernel32.inc
    include c:\masm32\include\advapi32.inc

    includelib c:\masm32\lib\user32.lib
    includelib c:\masm32\lib\kernel32.lib
    includelib c:\masm32\lib\advapi32.lib
 


    Main        PROTO
    StrCmp      PROTO :DWORD,:DWORD
    GetCL       PROTO :DWORD,:DWORD
    StdOut      PROTO :DWORD
    htodw       PROTO :DWORD
    szCatStr    PROTO :DWORD,:DWORD

; =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

  .data
    lf          DB 13,10,0
    Msg1        DB 13,10,"RegTool v0.96 (c) 2002-2003 by Florian Muecke",13,10,13,10
                DB "usage: ""regtool.exe <add>    <HKEY> <SubKey> <ValueName> <Type> <Data>""",13,10
                DB "       ""regtool.exe <del>    <HKEY> <SubKey> <ValueName>""",13,10
                DB "       ""regtool.exe <delkey> <HKEY> <SubKey>""  (quiet mode)",13,10,13,10
                DB "HKEY - must be one of those (HKCR, HKCU, HKLM, HKU)",13,10
                DB "SubKey - your desired SubKey",13,10 
                DB "ValueName - the name of the desired value",13,10
                DB "Type - must be one of those (REG_DWORD, REG_NONE, REG_SZ, REG_BINARY)",13,10
                DB "Data - the Key Value",13,10,13,10
                DB "e.g. regtool.exe add HKLM Software\RegTool TestString REG_SZ ""This is a test!""",13,10
                DB "     regtool.exe add HKLM Software\RegTool dword REG_DWORD FF0101FF",13,10,0
                ;REG_EXPAND_SZ, REG_MULTI_SZ, REG_RESOURCE_LIST
    MsgAdd      DB "adding value ",0
    MsgDel      DB "removing value ",0
    MsgDelKey   DB "removing key ",0
    szAdd       DB "add",0
    szDel       DB "del",0
    szDelKey    DB "delkey",0
    szHKLM      DB "HKLM",0
    szHKCU      DB "HKCU",0
    szHKCR      DB "HKCR",0
    szHKU       DB "HKU",0
    szREG_SZ    DB "REG_SZ",0
    szREG_DWORD DB "REG_DWORD",0
    szREG_NONE  DB "REG_NONE",0
    szREG_BINARY DB "REG_BINARY",0
    tmp_dd      DD 0
    DataLen     DD 0

; =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

  .code

  start:
    invoke Main
    invoke ExitProcess,0

; =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

Main proc

    LOCAL Cmd[7]:BYTE
    LOCAL HKey[5]:BYTE
    LOCAL SubKey[256]:BYTE
    LOCAL ValueName[128]:BYTE
    LOCAL Data[256]:BYTE
    LOCAL RegType[10]:BYTE
    LOCAL phkResult:DWORD
    LOCAL lpdwDisposition:DWORD
    LOCAL RegTypeVal:DWORD
    LOCAL DataLength:DWORD
    LOCAL HKeyVal:DWORD
    LOCAL OutBuffer[256]:BYTE
    LOCAL DataBuffer[256]:BYTE
    LOCAL ByteVal:BYTE

    invoke GetCL,1,addr Cmd
    .if eax != 1
        jmp error
    .endif
    invoke GetCL,2,addr HKey
    .if eax != 1
        jmp error
    .else
        invoke StrCmp,addr szHKLM,addr HKey
        .if eax == 0
            mov HKeyVal,080000002h;HKEY_LOCAL_MACHINE
        .else
            invoke StrCmp,addr szHKCU, addr HKey
            .if eax == 0
                mov HKeyVal,080000001h;HKEY_CURRENT_USER
            .else
                invoke StrCmp,addr szHKCR, addr HKey
                .if eax == 0
                    mov HKeyVal,080000000h;HKEY_CLASSES_ROOT
                .else
                    invoke StrCmp,addr szHKU, addr HKey
                    .if eax == 0
                        mov HKeyVal,080000003h;HKEY_USERS
                    .else
                        jmp error
                    .endif
                .endif
            .endif
        .endif
    .endif
    invoke GetCL,3,addr SubKey
    .if eax != 1
        jmp error
    .endif
  ;add
    invoke StrCmp, addr Cmd, addr szAdd
    .if eax == 0
        invoke GetCL,4,addr ValueName
;        .if eax != 1
;            jmp error
;        .endif
;can also be emty
        invoke GetCL,6,addr Data
;        .if eax != 1
;            jmp error
;        .endif
;can also be emty
        invoke GetCL,6,addr DataBuffer
        invoke GetCL,5,addr RegType
        .if eax != 1
            jmp error
        .else
            invoke StrCmp,addr szREG_SZ,addr RegType
            .if eax == 0
                mov RegTypeVal,REG_SZ
                invoke lstrlen,addr Data
                inc eax ;include 0
                mov DataLength,eax
            .else
                invoke StrCmp,addr szREG_DWORD,addr RegType
                .if eax == 0
                    mov RegTypeVal,REG_DWORD
                    mov DataLength,4
                    invoke htodw,addr DataBuffer
                    lea edi,DataBuffer
                    stosd
                .else
                    invoke StrCmp,addr szREG_BINARY,addr RegType
                    .if eax == 0
                        mov RegTypeVal,REG_BINARY
                        invoke GetCL,6,addr Data
                        invoke lstrlen,addr Data
                        inc eax ;simulate ending comma
                        mov ebx,3
                        div bl
                        mov DataLength,eax
                        
                        lea esi,Data
                    	lea edi,DataBuffer
                    binloop:
                        lodsb			;get first nibble of byte in al
                        .if al <= "9"
                            sub al,030h
                        .elseif al <= "F"
                            sub al,037h
                        .else
                            sub al,057h
                        .endif
                        mov bl,16
                        mul bl
                        mov ByteVal,al
                        lodsb			;get second nibble of byte in al
                        .if al <= "9"
                            sub al,030h
                        .elseif al <= "F"
                            sub al,037h
                        .else
                            sub al,057h
                        .endif
                        add ByteVal,al
                        mov al,ByteVal
                        stosb
                        inc esi				;skip komma
                        inc DataLen
                        mov eax, DataLength
                        .if DataLen < eax
                            jmp binloop
                        .endif
                    .else
                        invoke StrCmp,addr szREG_NONE,addr RegType
                        .if eax == 0
                            mov RegTypeVal,REG_NONE
                            mov DataLength,0
                        .else
                            jmp error
                        .endif
                    .endif
                .endif
            .endif
        .endif
        invoke RegCreateKeyEx,HKeyVal,addr SubKey,NULL,NULL,
               NULL,KEY_ALL_ACCESS,NULL,addr phkResult,addr lpdwDisposition
        invoke RegSetValueEx,phkResult,addr ValueName,0,RegTypeVal,addr DataBuffer,DataLength
        invoke RegCloseKey, phkResult
      ;preparing out message
        invoke szCatStr,addr OutBuffer,addr MsgAdd
        invoke szCatStr,addr OutBuffer,addr HKey
        mov tmp_dd,05ch ;"\"
        invoke szCatStr,addr OutBuffer,addr tmp_dd
        invoke szCatStr,addr OutBuffer,addr SubKey
        invoke szCatStr,addr OutBuffer,addr tmp_dd
        invoke szCatStr,addr OutBuffer,addr ValueName
        mov tmp_dd,"="
        invoke szCatStr,addr OutBuffer,addr tmp_dd
        invoke szCatStr,addr OutBuffer,addr Data
        invoke szCatStr,addr OutBuffer,addr lf
        invoke StdOut,addr OutBuffer
        jmp the_end
    .endif
  ;del
    invoke StrCmp, addr Cmd, addr szDel
    .if eax == 0
        invoke GetCL,4,addr ValueName
        .if eax != 1
            jmp error
        .endif
        invoke RegCreateKeyEx,HKeyVal,addr SubKey,NULL,NULL,
               NULL,KEY_ALL_ACCESS,NULL,addr phkResult,addr lpdwDisposition
        invoke RegDeleteValue,phkResult,addr ValueName
        invoke RegCloseKey, phkResult
      ;preparing out message
        invoke szCatStr,addr OutBuffer,addr MsgDel
        invoke szCatStr,addr OutBuffer,addr HKey
        mov tmp_dd,05ch ;"\"
        invoke szCatStr,addr OutBuffer,addr tmp_dd
        invoke szCatStr,addr OutBuffer,addr SubKey
        invoke szCatStr,addr OutBuffer,addr tmp_dd
        invoke szCatStr,addr OutBuffer,addr ValueName
        invoke szCatStr,addr OutBuffer,addr lf
        invoke StdOut,addr OutBuffer
        jmp the_end
    .endif
  ;delkey
    invoke StrCmp, addr Cmd, addr szDelKey
    .if eax == 0
        invoke RegDeleteKey,HKeyVal,addr SubKey
      ;preparing out message
;;        invoke szCatStr,addr OutBuffer,addr MsgDelKey
;;        invoke szCatStr,addr OutBuffer,addr HKey
;;        mov tmp_dd,05ch ;"\"
;;        invoke szCatStr,addr OutBuffer,addr tmp_dd
;;        invoke szCatStr,addr OutBuffer,addr SubKey
;;        invoke szCatStr,addr OutBuffer,addr lf
;;        invoke StdOut,addr OutBuffer
        jmp the_end
    .endif
    jmp error
  error:
    invoke StdOut, addr Msg1
  the_end:
    ret

Main endp

; =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
;  compare Strings and return 0 if equal, -1 if unequal
StrCmp PROC lpSrc:DWORD, lpDest:DWORD

    invoke lstrlen, lpSrc
    push eax
    invoke lstrlen, lpDest
    pop ebx
    .if eax != ebx
        mov eax, -1
        ret
    .endif
    mov ecx, eax
    mov edi, lpSrc
    mov esi, lpDest
    repe cmpsb
    .if ecx == 0
        xor eax,eax
        ret
    .endif
    mov eax, -1
    ret

StrCmp ENDP

; =*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
GetCL proc ArgNum:DWORD, ItemBuffer:DWORD

  ; -------------------------------------------------
  ; arguments returned in "ItemBuffer"
  ;
  ; arg 0 = program name
  ; arg 1 = 1st arg
  ; arg 2 = 2nd arg etc....
  ; -------------------------------------------------
  ; Return values in eax
  ;
  ; 1 = successful operation
  ; 2 = no argument exists at specified arg number
  ; 3 = non matching quotation marks
  ; 4 = empty quotation marks
  ; -------------------------------------------------

    LOCAL lpCmdLine      :DWORD
    LOCAL cmdBuffer[512] :BYTE
    LOCAL tmpBuffer[512] :BYTE

    push esi
    push edi

    invoke GetCommandLine
    mov lpCmdLine, eax        ; address command line

  ; -------------------------------------------------
  ; count quotation marks to see if pairs are matched
  ; -------------------------------------------------
    xor ecx, ecx            ; zero ecx & use as counter
    mov esi, lpCmdLine
    
    @@:
      lodsb
      cmp al, 0
      je @F
      cmp al, 34            ; [ " ] character
      jne @B
      inc ecx               ; increment counter
      jmp @B
    @@:

    push ecx                ; save count

    shr ecx, 1              ; integer divide ecx by 2
    shl ecx, 1              ; multiply ecx by 2 to get dividend

    pop eax                 ; put count in eax
    cmp eax, ecx            ; check if they are the same
    je @F
      pop edi
      pop esi
      mov eax, 3            ; return 3 in eax = non matching quotation marks
      ret
    @@:

  ; ------------------------
  ; replace tabs with spaces
  ; ------------------------
    mov esi, lpCmdLine
    lea edi, cmdBuffer

    @@:
      lodsb
      cmp al, 0
      je rtOut
      cmp al, 9     ; tab
      jne rtIn
      mov al, 32
    rtIn:
      stosb
      jmp @B
    rtOut:
      stosb         ; write last byte

  ; -----------------------------------------------------------
  ; substitute spaces in quoted text with replacement character
  ; -----------------------------------------------------------
    lea eax, cmdBuffer
    mov esi, eax
    mov edi, eax

    subSt:
      lodsb
      cmp al, 0
      jne @F
      jmp subOut
    @@:
      cmp al, 34
      jne subNxt
      stosb
      jmp subSl     ; goto subloop
    subNxt:
      stosb
      jmp subSt

    subSl:
      lodsb
      cmp al, 32    ; space
      jne @F
        mov al, 254 ; substitute character
      @@:
      cmp al, 34
      jne @F
        stosb
        jmp subSt
      @@:
      stosb
      jmp subSl

    subOut:
      stosb         ; write last byte

  ; ----------------------------------------------------
  ; the following code determines the correct arg number
  ; and writes the arg into the destination buffer
  ; ----------------------------------------------------
    lea eax, cmdBuffer
    mov esi, eax
    lea edi, tmpBuffer

    mov ecx, 0          ; use ecx as counter

  ; ---------------------------
  ; strip leading spaces if any
  ; ---------------------------
    @@:
      lodsb
      cmp al, 32
      je @B

    l2St:
      cmp ecx, ArgNum     ; the number of the required cmdline arg
      je clSubLp2
      lodsb
      cmp al, 0
      je cl2Out
      cmp al, 32
      jne cl2Ovr           ; if not space

    @@:
      lodsb
      cmp al, 32          ; catch consecutive spaces
      je @B

      inc ecx             ; increment arg count
      cmp al, 0
      je cl2Out

    cl2Ovr:
      jmp l2St

    clSubLp2:
      stosb
    @@:
      lodsb
      cmp al, 32
      je cl2Out
      cmp al, 0
      je cl2Out
      stosb
      jmp @B

    cl2Out:
      mov al, 0
      stosb

  ; ------------------------------
  ; exit if arg number not reached
  ; ------------------------------
    .if ecx < ArgNum
      mov edi, ItemBuffer
      mov al, 0
      stosb
      mov eax, 2  ; return value of 2 means arg did not exist
      pop edi
      pop esi
      ret
    .endif

  ; -------------------------------------------------------------
  ; remove quotation marks and replace the substitution character
  ; -------------------------------------------------------------
    lea eax, tmpBuffer
    mov esi, eax
    mov edi, ItemBuffer

    rqStart:
      lodsb
      cmp al, 0
      je rqOut
      cmp al, 34    ; dont write [ " ] mark
      je rqStart
      cmp al, 254
      jne @F
      mov al, 32    ; substitute space
    @@:
      stosb
      jmp rqStart

  rqOut:
      stosb         ; write zero terminator

  ; ------------------
  ; handle empty quote
  ; ------------------
    mov esi, ItemBuffer
    lodsb
    cmp al, 0
    jne @F
    pop edi
    pop esi
    mov eax, 4  ; return value for empty quote
    ret
  @@:

    mov eax, 1  ; return value success

    pop edi
    pop esi

    ret

GetCL endp

;---------------------------------------------------------------------
StdOut proc lpszText:DWORD

    LOCAL hOutPut  :DWORD
    LOCAL bWritten :DWORD
    LOCAL sl       :DWORD

    invoke GetStdHandle,STD_OUTPUT_HANDLE
    mov hOutPut, eax

    invoke lstrlen,lpszText
    mov sl, eax

    invoke WriteFile,hOutPut,lpszText,sl,ADDR bWritten,NULL

    mov eax, bWritten
    ret

StdOut endp

;---------------------------------------------------------------------
htodw proc String:DWORD

  ; -----------------------------------
  ; Convert hex string into dword value
  ; Return value in eax
  ; -----------------------------------

    push ebx
    push esi
    push edi

    mov edi, String
    mov esi, String 

     ALIGN 4

  again:  
    mov al,[edi]
    inc edi
    or  al,al
    jnz again
    sub esi,edi
    xor ebx,ebx
    add edi,esi
    xor edx,edx
    not esi             ;esi = lenth

  .while esi != 0
    mov al, [edi]
    cmp al,'A'
    jb figure
    sub al,'a'-10
    adc dl,0
    shl dl,5            ;if cf set we get it bl 20h else - 0
    add al,dl
    jmp next
  figure: 
    sub al,'0'
  next:  
    lea ecx,[esi-1]
    and eax, 0Fh
    shl ecx,2           ;mul ecx by log 16(2)
    shl eax,cl          ;eax * 2^ecx
    add ebx, eax
    inc edi
    dec esi
  .endw

    mov eax,ebx

    pop edi
    pop esi
    pop ebx

    ret

htodw endp

;---------------------------------------------------------------------
szCatStr proc lpszSource:DWORD, lpszAdd:DWORD

    invoke lstrlen,lpszSource
    mov edx, lpszSource
    mov ecx, lpszAdd
    add edx, eax

  @@:
    mov al, [ecx]
    mov [edx], al
    inc ecx
    inc edx
    test al, al       ; test for zero
    jne @B

    ret

szCatStr endp
;---------------------------------------------------------------------

end start