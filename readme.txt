IMBREA GIULIA-STEFANIA 321CB
TEMA2PCOM

OBS: 
- mereu inainte de a testa trb sa dau wsl --shutdown!!!!
- daca rulez fara sudo am 19/20 teste (imi pica cel de ..)
	proof: https://imgur.com/a/fGmufwz
-daca rulez cu sudo am eroare
	proof: https://imgur.com/a/ndv0Z0f
     
-local imi merge orice combinatie
-am reincarcat tema cu o zi intarziere pt ca initial aveam 15/20, sper sa merite =)))


server.c
- in main la inceput imi setez structura sockaddr_in server_addr pemtru
a fi receptiv pe portul dat ca argument
- fac urmatorii pai de setare a unui socket: socket, bind, listen
- folosesc multiplexarea cu poll
- imi setez in vectorul poll pe pozitia 0-tastatura, 1-mesaje tcp, 
2-mesaje udp, >3 fd clientilor

1. pt tastatura
- poate primi doar mesaj de exit
daca citesc mesajul de exit, inchid fiecare fd
2. pt tcp
- imi setez structurile sockaddr_in pt client'
- imi creez un  nou socket cu accept
- adaug socketul creat la lista de poll uri
- acum trebuie sa verific daca clientul exista deja, daca nu, il creez
3. pt udp
- primesc mesajul cu recvfrom
- imi construiesc un mesaj de tip udp si il populez
- pt fiecare subscriber abonat la topicul respectiv trimit mesajul udp
4. pt clienti 

-construiesc mesaj tco
-daca rezultatul la recv == 0 => trebuie deconectat subscriberul
si sters fd ul lui din poll
- pentru a identifica tipul comenzii, m am folosit de prima litera:
s pt subcribe si u pt unsubscribe
- daca campul cmd din mesaj este : 
    s: verific mai intai daca topicul mentionat exista deja, si daca da 
    adaug un nou subscriber la el, altfel creez un nou topic
    u: efectiv din lista de topicuri, ma opresc la topicul sprecificat si sterg
    subscriber ul


subscriber.c
-prima parte este asemanatoare cu cea de la server.c doar ca
vectorul cu poll uri are doar 2 fd uri: tastatura/STDIN si socketul de client

1. tastatura
- daca citesc exit=> inchid socketul clientului
- altfel => mesaj tcp
- populez campurile din tcp_msg
2. udp
- primesc cu recv
- daca rezultatul la recv == 0 inchid socketul clientului
- interpretarea mesajului: int/float/string
=> aaici am respectat cerinta
pana la data+50 se afla topicul si dupa in functie de tipul de data scot elementele necesare
de ex: la 51 (8 biti)pt int am sign ul nr ului


