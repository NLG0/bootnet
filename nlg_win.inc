call	[ExitProcess]


;*****************************************************************************
;sous fonctions g�n�rale



ouvre_fichier:
;entr�e: ds:edx=nom du fichier a ouvrir
;sortie  eax=0 si ok   ebx=handle du fichier ouvert


push	dword 0              ;
push	dword 0              ;
push	dword 3              ;open existing
push	dword 0              ;
push	dword 0              ;autorisation pour les autres programmes qui chercherait a ouvrir ce fichier pour le lire
push	0C0000000h      ;acc�s lecture + ecriture 
push	edx
call	[CreateFile]
cmp	eax,-1
je err_ouvre_fichier
mov ebx,eax
xor eax,eax
ret

err_ouvre_fichier:
xor ebx,ebx
ret


cree_fichier:
;entr�e: ds:edx=nom du fichier a cr�er
;sortie  eax=0 si ok   ebx=handle du fichier cr�e

push	dword 0              ;
push	dword 0              ;
push	dword 2              ;create always
push	dword 0              ;
push	dword 0              ;autorisation pour les autres programmes qui chercherait a ouvrir ce fichier
push	40000000h      ;acc�s (ecriture seule) 
push	edx
call	[CreateFile]
cmp	eax,-1
je err_ouvre_fichier
mov ebx,eax
xor eax,eax
ret



ferme_fichier:
push ebx
call [CloseHandle]
ret



lit_fichier:
;ebx=handle
;ecx=quantit�
;edx=offset dans le fichier
;edi=adresse de destination

pushad
push	dword 0    ;a partir de quel endroit du fichier. 0=debut du fichier 1=a partir de la position actuelle du pointeur 2=la fin du fichier
mov dword[dword_temporaire],0  
push	dword_temporaire      ;pointeur dword MSB ou Null
push	edx    ;LSB de l'adresse
push	ebx    ;handle
call	[SetFilePointer]
popad

push	dword 0       ;null
push	dword_temporaire ;adresse ou ecrire la quantit� de donn�es lue
push	ecx     ;quantit� de donn�es a lire
push	edi     ;adresse ou ecrire les donn�es lue  
push	ebx     ;handle du fichier
call	[ReadFile]
or	eax,eax
jz	erreur_le
clc
ret

ecr_fichier:
;ebx=handle
;ecx=quantit�
;edx=offset dans le fichier
;esi=adresse de source

pushad
push	dword 0    ;a partir de quel endroit du fichier. 0=debut du fichier 1=a partir de la position actuelle du pointeur 2=la fin du fichier
mov dword[dword_temporaire],0  
push	dword_temporaire      ;pointeur sur dword MSB
push	edx    ;LSB de l'adresse
push	ebx    ;handle
call	[SetFilePointer]
popad

push	dword 0       ;null
push	dword_temporaire ;adresse ou ecrire la quantit� de donn�es lue
push	ecx     ;quantit� de donn�es a lire
push	esi     ;adresse ou se trouve les donn�es a ecrire lue  
push	ebx     ;handle du fichier
call	[WriteFile]
or	eax,eax
jz	erreur_le
clc
ret

erreur_le:
stc
ret



taillef: ;ebx=handle
;sortie: ecx=taille
push dword dword_temporaire      ;pointeur sortie MSB de la taille
push ebx    ;handle
call	[GetFileSize]
mov ecx,eax
ret



affmsg: ;affiche la chaine asciiz point� par edx
push	dword 0FFFFFFF5h
call	[GetStdHandle]     ;r�cup�re handle de la sortie console
mov	ebp,eax
mov	edi,edx
or	ecx,-1
xor	al,al
repne	scasb  
neg	ecx
sub	ecx,2
push	0                 ;null 
push	dword dword_temporaire ;sortie nombre de byte ecrit
push	ecx               ;nb d'octet a ecrire
push	edx               ;adresse ou lire les donn�es a ecrire      
push	ebp               ;handle du fichier ou ecrire 
call	[WriteFile]
ret



conv_nombre:
mov al,"$"
push eax
mov eax,ecx
mov ecx,10

boucle_conv_nombre1:
xor edx,edx
div ecx
add dl,"0"
push edx
cmp eax,0
jne boucle_conv_nombre1

boucle_conv_nombre2:
pop eax
cmp al,"$"
je fin_conv_nombre
mov [esi],al
inc esi
jmp boucle_conv_nombre2


fin_conv_nombre:
mov byte[esi],0
ret



;******************************************************************

net_init:
pushad
push dword WSADATA
push dword 101h       ;version de winsock a initialiser (1.1 ici)
call	[WSAStartup]  ;initialisation de winsock

ss
mov [esp+28],eax   ;pr�serve eax
popad
ret

;********************
net_ecoute_tcp:    ;ouvre un port TCP en �coute
;entr�e        bx=num�ros de port
;              edx=adresse de l'inteface sur laquel ouvrir le port ou adresse ip null (edx=0), pour ouvrir le port sur toutes les interface r�seau
;sortie        eax=code d'erreur ebx=handle du nouveau port

pushad
mov word[addr_ip],2
mov [addr_ip+2],bh
mov [addr_ip+3],bl

mov [addr_ip+4],edx  ;adresse ip
xor ebx,ebx
mov [addr_ip+8],ebx  ;zone toujour nul
mov [addr_ip+12],ebx  ;zone toujour nul

;ouverture et configuration du port
push dword 6       ;protocol 1=icmp(pour ipv4) 6=tcp 17=udp 58=icmp(pour ipv6)
push dword 1   ;type de transmission (1 pour tcp et 2 pour udp)
push dword 2   ;adresse family 2=IPV4 23=IPV6
call [socket]  ;cr�e un socket
cmp eax,-1
je net_erreur 

mov ebx,eax

push dword 16
push dword addr_ip 
push ebx
call [bind]      ;definit l'adresse du socket
cmp eax,0
jne net_erreur

push dword 100     ;nombre de connexion maximum
push ebx
call [listen]      ;met le socket en mode �coute
cmp eax,0
jne net_erreur

mov dword[dword_temporaire],1 
push dword dword_temporaire  ;point sur un dword a 1
push dword 8004667Eh         ;commande "FIONBIO"
push ebx
call [ioctlsocket]  ;passe le socket en "non blocking mode"
cmp eax,0
jne net_erreur

ss
mov [esp+16],ebx  ;pr�serve ebx
popad
xor eax,eax
ret




net_erreur:
call [WSAGetLastError]
ss
mov [esp+16],eax  ;modifie ebx dans la pile
popad
mov eax,1
ret


;********************
net_connexion_tcp:    ;ouvre une connexion vers un port TCP
;entr�e        bx=num�ros de port source
;              cx=port de destination
;              edx=adresse de desitnation
;sortie        eax=code d'erreur ebx=handle du nouveau port

pushad

mov word[addr_ip],2
mov [addr_ip+2],bh
mov [addr_ip+3],bl
xor ebx,ebx
mov [addr_ip+4],ebx  ;adresse ip null, pour ouvrir le port sur toutes les interface r�seau sinon sp�cifier l'adresse de la seule carte qui doit ouvrir le port
mov [addr_ip+8],ebx   ;zone toujours nul
mov [addr_ip+12],ebx  ;zone toujours nul

mov word[addr_ip2],2
mov [addr_ip2+2],ch
mov [addr_ip2+3],cl
mov [addr_ip2+4],edx  
xor ebx,ebx
mov [addr_ip2+8],ebx   ;zone toujours nul
mov [addr_ip2+12],ebx  ;zone toujours nul



;ouverture et configuration du port
push dword 6   ;protocol 1=icmp(pour ipv4) 6=tcp 17=udp 58=icmp(pour ipv6)
push dword 1  ;type de transmission (1 pour tcp et 2 pour udp)
push dword 2   ;adresse family 2=IPV4 23=IPV6
call [socket]  ;cr�e un socket
cmp eax,-1
je net_erreur 

mov ebx,eax

push dword 16
push dword addr_ip 
push ebx
call [bind]      ;definit l'adresse du socket
cmp eax,0
jne net_erreur


mov dword[dword_temporaire],16
push dword dword_temporaire  ;zone de 16
push dword addr_ip2           ;adresse de la connexion
push ebx
call [connect]
cmp eax,0
jne net_erreur
popad
xor eax,eax
ret





;********************
net_test_nvc:   ;test si une nouvelle connexion est disponible

pushad
mov dword[dword_temporaire],16
push dword dword_temporaire  ;zone de 16
push dword addr_ip          ;adresse de la connexion
push ebx
call [accept]
cmp eax,-1
je erreur_nouv_connexion

ss
mov [esp+16],eax  ;modifie ebx dans la pile
popad
mov eax,1
ret

erreur_nouv_connexion:
call [WSAGetLastError]
cmp eax,10035   ;code d'erreur "WSAEWOULDBLOCK"
je aucune_nouv_connexion

ss
mov [esp+16],eax  ;modifie ebx dans la pile
popad
mov eax,2
ret 

aucune_nouv_connexion:
popad
xor eax,eax
xor ebx,ebx
ret 



;********************
net_lire_port_tcp:           ;recevoir donn�es sur port tcp
;entr�e
;ecx=quantit� a re�evoire
;edi=zone ou ecrire les donn�es
;ebx= handle du port a lire
;sortie
;ecx=nombre de donn�es donn�es lut

pushad

push ebx
push ecx
push edi
mov dword[dword_temporaire],1 
push dword dword_temporaire  ;point sur un dword a 1
push dword 8004667Eh         ;commande "FIONBIO"
push ebx
call [ioctlsocket]  ;passe le socket en "non blocking mode"
pop edi
pop ecx
pop ebx
cmp eax,0
jne erreur_lire_port

push dword 0
push ecx
push edi
push ebx
call [recv]
cmp eax,-1
je erreur_lire_port

ss
mov [esp+24],eax  ;modifie ecx dans la pile
popad
xor eax,eax
xor ebx,ebx
ret


erreur_lire_port:
call [WSAGetLastError]
cmp eax,10035   ;code d'erreur "WSAEWOULDBLOCK"
je aucune_donnee_lut

ss
mov [esp+16],eax  ;pr�serve ebx
popad
mov eax,2
ret 

aucune_donnee_lut:
popad
xor eax,eax
xor ecx,ecx
ret 






;********************
net_ecrire_port_tcp:
;envoyer donn�es
;ecx=quantit� a envoyer
;esi=zone ou se trouve les donn�es a envoyer
;ebx=handle du port a lire

pushad

push ebx
push ecx
push esi
mov dword[dword_temporaire],0 
push dword dword_temporaire  ;point sur un dword a 0
push dword 8004667Eh         ;commande "FIONBIO"
push ebx
call [ioctlsocket]  ;passe le socket en "blocking mode"
pop esi
pop ecx
pop ebx
cmp eax,0
jne erreur_lire_port

push dword 0
push ecx
push esi
push ebx
call [send]
cmp eax,-1
je erreur_lire_port

ss
mov [esp+24],eax  ;modifie ecx dans la pile
popad
xor eax,eax
ret


;*********************************************************************************
net_ouvre_port_udp:
;entr�e        bx=num�ros de port
;              ebp=adresse de l'inteface sur laquel utiliser le port ou adresse ip null (ebp=0), pour ouvrir le port sur toutes les interface r�seau
;sortie        eax=code d'erreur ebx=handle du nouveau port

pushad
mov word[addr_ip],2
mov [addr_ip+2],bh
mov [addr_ip+3],bl

mov [addr_ip+4],ebp  ;adresse ip
xor eax,eax
mov [addr_ip+8],eax  ;zone toujour nul
mov [addr_ip+12],eax  ;zone toujour nul

;ouverture et configuration du port
push dword 17       ;protocol 1=icmp(pour ipv4) 6=tcp 17=udp 58=icmp(pour ipv6)
push dword 2   ;type de transmission (1 pour tcp et 2 pour udp)
push dword 2   ;adresse family 2=IPV4 23=IPV6
call [socket]  ;cr�e un socket
cmp eax,-1
je net_erreur 

push eax
push dword 16
push dword addr_ip 
push eax
call [bind]      ;definit l'adresse du socket
pop ebx
cmp eax,0
jne net_erreur

push ebx
mov dword[dword_temporaire],1 
push dword 1
push dword dword_temporaire  ;point sur un dword a 1
push dword 32  ;nom de l'option 32=SO_BROADCAST
push dword 0FFFFh ;niveau SOL_SOCKET
push ebx
call [setsockopt]
pop ebx
cmp eax,0
jne net_erreur

ss
mov [esp+16],ebx  ;modifie ebx dans la pile
popad
xor eax,eax
ret


;****************************************************
net_lire_port_udp:           ;recevoir donn�es sur port udp
;entr�e
;ecx=quantit� a re�evoire
;edi=zone ou ecrire les donn�es
;ebx= handle du port a lire
;sortie
;ecx=nombre de donn�es donn�es lut
;edx=port ip d'origine de la trame
;ebp=adresse ip d'origine de la trame

pushad

push ebx
push ecx
push edi
mov dword[dword_temporaire],1 
push dword dword_temporaire  ;point sur un dword a 1
push dword 8004667Eh         ;commande "FIONBIO"
push ebx
call [ioctlsocket]  ;passe le socket en "non blocking mode"
pop edi
pop ecx
pop ebx
cmp eax,0
jne erreur_lire_port

mov dword[dword_temporaire],16
push dword dword_temporaire  ;zone de 16
push dword addr_ip          ;adresse de la connexion
push dword 0
push ecx
push edi
push ebx
call [recvfrom]
cmp eax,-1
je erreur_lire_port

  
ss
mov [esp+24],eax  ;modifie ecx dans la pile
xor eax,eax
mov ah,[addr_ip+2]
mov al,[addr_ip+3]
ss
mov [esp+20],eax  ;modifie edx dans la pile
mov eax,[addr_ip+4]
ss
mov [esp+8],eax  ;modifie ebp dans la pile

popad
xor eax,eax
ret


;********************************
net_ecrire_port_udp:
;dx= num�ros de port
;ebp=adresse
;ecx=quantit� a envoyer
;esi=zone ou se trouve les donn�es a envoyer
;ebx=handle du port a lire

pushad

mov word[addr_ip],2
mov [addr_ip+2],dh
mov [addr_ip+3],dl

mov [addr_ip+4],ebp  ;adresse ip
xor eax,eax
mov [addr_ip+8],eax  ;zone toujour nul
mov [addr_ip+12],eax  ;zone toujour nul

push ebx
push ecx
push esi
mov dword[dword_temporaire],0 
push dword dword_temporaire  ;point sur un dword a 0
push dword 8004667Eh         ;commande "FIONBIO"
push ebx
call [ioctlsocket]  ;passe le socket en "blocking mode"
pop esi
pop ecx
pop ebx
cmp eax,0
jne net_erreur


mov dword[dword_temporaire],16
push dword dword_temporaire  ;zone de 16
push dword addr_ip          ;adresse de la connexion
push dword 0
push ecx
push esi
push ebx
call [sendto]
cmp eax,-1
je net_erreur

popad
xor eax,eax
xor ebx,ebx
ret










;**************************************************************************
;donn�es de relocation des appels de sous fonctions

section '.idata' import data readable writeable

  dd 0,0,0,rva kernel32_name,rva kernel32_table 
  dd 0,0,0,rva user32_name,rva user32_table      
  dd 0,0,0,rva winsock_name,rva winsock_table
  dd 0,0,0,0,0



kernel32_table:                                 ;relocation des fonctions de KERNEL32.DLL
    ExitProcess dd rva _ExitProcess
    CreateFile dd rva _CreateFileA
    ReadFile dd rva _ReadFile
    WriteFile dd rva _WriteFile
    CloseHandle dd rva _CloseHandle
    GetFileSize dd rva _GetFileSize
    SetFilePointer dd rva _SetFilePointer
    GetCommandLine dd rva _GetCommandLineA
    GetStdHandle dd rva _GetStdHandle
    SwitchToThread dd rva _SwitchToThread
    Sleep dd rva _Sleep
    dd 0

kernel32_name db 'KERNEL32.DLL',0
    _ExitProcess dw 0
    db 'ExitProcess',0
    _CreateFileA dw 0
    db 'CreateFileA',0
    _ReadFile dw 0
    db 'ReadFile',0
    _WriteFile dw 0
    db 'WriteFile',0
    _CloseHandle dw 0
    db 'CloseHandle',0
    _GetFileSize dw 0
    db 'GetFileSize',0
     _SetFilePointer dw 0
    db 'SetFilePointer',0
    _GetCommandLineA dw 0
    db 'GetCommandLineA',0
    _GetStdHandle dw 0
    db 'GetStdHandle',0
    _SwitchToThread dw 0
    db 'SwitchToThread',0
    _Sleep dw 0
    db 'Sleep',0


user32_table:                                 ;relocation des fonctions de USER32.DLL
    GetKeyState dd rva _GetKeyState
    GetKeyboardState dd rva _GetKeyboardState
    dd 0

user32_name db 'USER32.DLL',0
    _GetKeyState dw 0
    db 'GetKeyState',0
    _GetKeyboardState dw 0
    db 'GetKeyboardState',0



winsock_table:                               ;relocation des fonctions de WS2_32.DLL
    accept dd rva _accept
    bind dd rva _bind
    connect dd rva _connect
    ioctlsocket dd rva _ioctlsocket
    listen dd rva _listen
    recv dd rva _recv
    recvfrom dd rva _recvfrom
    send dd rva _send
    sendto dd rva _sendto
    setsockopt dd rva _setsockopt
    socket dd rva _socket
    WSAGetLastError dd rva _WSAGetLastError
    WSACleanup dd rva _WSACleanup
    WSAStartup dd rva _WSAStartup
    dd 0
 
winsock_name db 'WS2_32.DLL',0

    _accept dw 0
    db 'accept',0
    _bind dw 0
    db 'bind',0
    _connect dw 0
    db 'connect',0
    _ioctlsocket dw 0
    db 'ioctlsocket',0
    _listen dw 0
    db 'listen',0
    _recv dw 0
    db 'recv',0
    _recvfrom dw 0
    db 'recvfrom',0
    _send dw 0
    db 'send',0
    _sendto dw 0
    db 'sendto',0
    _setsockopt dw 0
    db 'setsockopt',0
    _socket dw 0
    db 'socket',0
    _WSAGetLastError dw 0
    db 'WSAGetLastError',0
    _WSACleanup dw 0
    db 'WSACleanup',0
    _WSAStartup dw 0
    db 'WSAStartup',0




section '.reloc' fixups data readable discardable



;***********************************************************
;donn�es du programme

section '.data' data readable writeable

dword_temporaire:
dd 0



;structure adresse IP




WSADATA:
addr_ip:
dw 2       ;type d'adresse 2=adresse ip V4
dw 0       ;port avec msb et lsb invers�
db 0,0,0,0 ;adresse ip V4
dd 0,0     ;toujours vide
addr_ip2:

dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 ;84
dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 ;100
dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 ;100
dd 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 ;100

 