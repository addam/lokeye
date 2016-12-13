# Aktuální plán a TODO list

poslední změna: 13. prosince

## Sledování obličeje ve videu na disku
Místo streamu z kamery musí jít taky otevřít soubor a nechat si ho na ukázku zpracovat.

## Adaptive Shape Model
Trackování několika obdélníků v obrázku působí jako dost nahodilá volba.
Větší smysl dává, položit přes obrázek mřížku bodů a trackovat tu.
Jako alternativu klasické trojúhelníkové síti (barycentrické souřadnice) chci pracovat s obdélníkovými buňkami a uvnitř každé počítat skutečné perspektivní zobrazení; věřím, že díky příspěvku sousedních buněk to bude stabilní.
Vyžaduje to napočítat derivace homografie při posouvání jednoho ze čtyř kotevních bodů, což je pro mě zajímavá oblast pro průzkum.

Trackování jedné buňky mám hotové pro barycentrickou i perspektivní transformaci.
Zbývá určit každému pixelu, do které buňky spadá, a správně sečíst výsledky sousedících buněk.

## Normalizace vůči světlosti a kontrastu
Jednak změny osvětlení a druhak automatické změny závěrky na webkameře můžou hledání obličeje docela zbourat. Normalizace světlosti uvnitř obličeje by měla tyhle problémy vyřešit. Asi nemá smysl tu normalizační funkci derivovat, to by bylo děsně složité; světlost obličeje změním před výpočtem jakoby mimochodem. Aby při posuvu obličeje nemohly výsledky moc skákat, posadím doprostřed obličeje gaussovský kernel a budu jím pixely vážit.

## Zohlednit víčka
Program hledá obrys duhovky (angl. limbus) jako kružnici s tmavým vnitřkem.
Přitom, i když se uživatel dívá skoro přímo do kamery, je zhruba půlka obrysu zakrytá víčky, a to patrně zkresluje výsledek.
Robustnější chybová funkce tedy na obrysu určí i váhovou funkci, která bude na víčku nulová.
Dobře by mohla fungovat barevná segmentace bělma.
Dále můžeme využít, že okraj duhovky má celý stejnou barvu, a tím rozlišit duhovku od víčka i z vnitřního okraje kružnice.

## Deterministická kalibrace
Aby šlo výpočet předvádět opakovaně s jednou natočeným videem, musí být možné napevno zvoli sekvenci fixací, jaká se ukazuje během kalibrace. Problém je se synchronizací; proto by možná bylo nejlepší údaje o kalibrační sekvenci i s čísly snímku někam uložit. Týž program může uložit i video z kamery, v OpenCV to není problém.

## Ukládání a načítání obličeje
Kalibrace trvá dlouho. Pro pohodlí by hodně prospělo si její výsledky uložit na disk a při následujícím spuštění programu je zkusit použít. Asi budou potřeba nějaké další heuristiky, protože se osvětlení může měnit dost nepředvidatelným způsobem.
