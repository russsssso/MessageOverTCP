Serverul comunica cu clientii cu prin protocolul Protocol.
Protocol contine sursa (clientul UPD care l a trimis) topicul, tipus de date si contentul.
Clientii comunica cu serverul prin protocolul Client_Message:
    contine msg_type (0 inseamna deconectare, 1 unsubscribe de la topic, 2 subscribe la topic)
    si data (100 bytes: data msg_type e 1 data e populata doar de topic, daca e 2 data are pe primul
    bit sf urmat de topic; daca e 0 e goala)

Partea de store and forward e realizata cu ajutorul unui hash table care contine Topicurile. Fiecare Topic are of
lista ilantuita de mesaje.
Clientii TCP sunt stocati intr un array de structuri TCP_Client. Structul stocheaza daca clientul este conectat,
pe ce fd/addresa si ce subscriptii are.

Atat HashTableul cat si Arrayul de clienti si cel de pollfd sunt reallocate in caz de atingerea dimensiunii maxime. (Nr de clienti/ mesaje nu e limitat)

Comunicarea peste TCP se face in calupuri fixe.
