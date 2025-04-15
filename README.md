# Sisteme-de-Operare
Proiectul propus reprezintă o aplicație software dezvoltată în limbajul C, având ca obiectiv analiza și monitorizarea fișierelor dintr-un set de directoare specificate de utilizator. 
Aplicația parcurge recursiv structura directorului, identifică fișierele potențial corupte sau periculoase, și generează fișiere de tip snapshot conținând atribute relevante ale fiecărui fișier (nume, dimensiune, permisiuni, data ultimei modificări, inode).

Identificarea fișierelor suspecte se realizează printr-un script auxiliar scris în Bash (verify_malicious.sh), care evaluează conținutul fișierelor pe baza unor tipare textuale specifice și a unor criterii statistice (număr de linii, cuvinte și caractere). În cazul în care un fișier este considerat periculos, acesta este automat mutat într-un director de izolare, protejând astfel restul sistemului.

Aplicația utilizează mecanisme avansate ale sistemului de operare, precum procese fiu, comunicare prin pipe-uri, apeluri de sistem POSIX (ex: lstat, open, read, write), și gestionează în mod robust excepțiile care pot apărea în timpul execuției. În plus, este prevăzută opțiunea de redirecționare a fișierelor de ieșire către un director specificat de utilizator, în vederea organizării și arhivării rezultatelor.
