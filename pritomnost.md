# Aktuální plán a TODO list

poslední změna: 22. června

## Text a data
* připravit (znovu) data na hledání očí
* vyhodnotit podrobně, jaká chyba obrázku způsobuje selhání kterého programu
* připravit videa uživatele, který má za úkol sledovat tečku na obrazovce

## Hledání očí
* přepsat programy z pískoviště do hlavní větve
* hlasování víc výpočtů běžících zároveň
* segmentace kůže a bělma
* neuronové sítě

## Hledání tváře
* inicializace přes opencv / adaboost
* mřížka pro barycentrickou transformaci
* mřížka pro perspektivitu
* normalizace osvětlení

## Gaze
* při kalibraci porovnávat aproximaci s aproximací konstantou
* pro mřížkové přístupy přidat možnost předzpracování vstupu pomocí PCA nebo LDA

# Odložené myšlenky

## Normalizace vůči světlosti a kontrastu
Jednak změny osvětlení a druhak automatické změny závěrky na webkameře můžou hledání obličeje docela zbourat. Normalizace světlosti uvnitř obličeje by měla tyhle problémy vyřešit. Asi nemá smysl tu normalizační funkci derivovat, to by bylo děsně složité; světlost obličeje změním před výpočtem jakoby mimochodem. Aby při posuvu obličeje nemohly výsledky moc skákat, posadím doprostřed obličeje gaussovský kernel a budu jím pixely vážit.

## Ukládání a načítání obličeje
Kalibrace trvá dlouho. Pro pohodlí by hodně prospělo si její výsledky uložit na disk a při následujícím spuštění programu je zkusit použít. Asi budou potřeba nějaké další heuristiky, protože se osvětlení může měnit dost nepředvidatelným způsobem.
