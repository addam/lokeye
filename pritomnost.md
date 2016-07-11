# Aktuální plán a TODO list

poslední změna: 10. července

## Optimalizaci nechat na knihovně Ceres
Ceres nabízí několik klasických algoritmů pro nelineární optimalizaci. Derivace si počítá sama, a to zčásti už během kompilace. Docela jistě prospěje přesnosti výpočtu, namísto metody největšího spádu s krokem pevné délky, jak ho používám teď.

Neprospěje. Výpočet nefunguje a kód je spíš ošklivější, než byl dřív, proto si optimalizaci zařídím sám.

## Zohlednit natočení hlavy
Můžeme sice předpokládat, že se uživatel dívá na monitor přímo, ale i malé otočení hlavy má na výsledky obrovský vliv. V neměnném směrovém osvětlení by mohlo stačit otáčení hlavy modelovat jako lineární kombinaci obrázků, na způsob eigenfaces. Rozlišení by mělo být dost hrubé, aby pohyb očí už nebyl vidět.

Nestačí. Při difúzním osvětlení má otočení převážně nelineární vliv, takže sbíráme samý šum.

Správnější přístup by byl odhadnout hloubkovou mapu a do modelu hlavy pak přidat ještě vliv optical flow. Při hrubém rozlišení by to mohlo být i docela stabilní. Řešily by se tak problémy s obroučkami brýlí, které akorát překážejí v oblasti u kořene nosu a nepředvídatelným způsobem čouhají do prostoru.

Robustní model natočení celého obličeje by zároveň měl poskytnout odhad toho, jak aktuálně vypadají vnitřní koutky očí. To může hodně prospět přesnosti výpočtu.

## Alternativa: sledovat pohyb nosu oproti očím
Vyžaduje to krom oblasti obličeje a očí vyznačit na začátku ještě oblast nosu. Výpočet pak vlastně je řídký optical flow, kde oči budou jeden landmark a nos bude druhý; při otáčení hlavy očekáváme, že vznikne paralaxa, a tou rovnou nakrmíme výpočet směru pohledu. Jako vedlejší efekt se tak zároveň vyřeší následující bod.

## Hledat cíleně vnitřní koutky očí
Porovnávání pohybu očí oproti celému obličeji je nesmírně nepřesné, když se uživatel začne divně tvářit. Z vlastního pozorování před zrcadlem vyplývá, že vnitřní koutky očí jsou docela pevně fixované na lebku (do textu diplomky opatřím podklady z učebnice anatomie). Aby výpočet zvládal prudké pohyby hlavy, hodí se ale celý obličej stejnak najít, jako první krok.

Potud hotovo.

## Afinní transformace obličeje
Shodnost funguje dobře, dokud se obličej hýbe jen v rovině kolmé na kameru. Otáčení hlavou má už citelné perspektivní zkreslení. Ze zkušeností s trackováním v Blenderu mám obavu, že perspektivita by se příliš pečlivě chytala na nepodstatné změny jako osvětlení a šířka úsměvu. Afinita má navíc skoro stejné vzorečky jako shodnost, takže bude radost ji optimalizovat.

## Normalizace vůči světlosti a kontrastu
Jednak změny osvětlení a druhak automatické změny závěrky na webkameře můžou hledání obličeje docela zbourat. Normalizace světlosti uvnitř obličeje by měla tyhle problémy vyřešit. Asi nemá smysl tu normalizační funkci derivovat, to by bylo děsně složité; světlost obličeje změním před výpočtem jakoby mimochodem. Aby při posuvu obličeje nemohly výsledky moc skákat, posadím doprostřed obličeje gaussovský kernel a budu jím pixely vážit.

## Zohlednit víčka
Program hledá obrys duhovky (angl. limbus) jako kružnici s tmavým vnitřkem. Přitom, i když se uživatel dívá skoro přímo do kamery, je zhruba půlka obrysu zakrytá víčky, a to patrně zkresluje výsledek. Robustnější chybová funkce pro oko by tedy měla na obrysu určit i váhovou funkci, která bude na víčku nulová. Jedno řešení je víčka explicitně modelovat, což by bylo užitečné i pro hledání koutků očí. O něco jednodušší možná bude předpokládat, že obrys duhovky má celý stejnou barvu, tu nějakým způsobem robustně odhadnout a na jejím základě klasifikovat každý pixel jednotlivě.

## Deterministická kalibrace
Aby šlo výpočet předvádět opakovaně s jednou natočeným videem, musí být možné napevno zvoli sekvenci fixací, jaká se ukazuje během kalibrace. Problém je se synchronizací; proto by možná bylo nejlepší údaje o kalibrační sekvenci i s čísly snímku někam uložit. Týž program může uložit i video z kamery, v OpenCV to není problém.

##takový program už by měl bohatě stačit.

## Ukládání a načítání obličeje
Kalibrace trvá dlouho. Pro pohodlí by hodně prospělo si její výsledky uložit na disk a při následujícím spuštění programu je zkusit použít. Asi budou potřeba nějaké další heuristiky, protože se osvětlení může měnit dost nepředvidatelným způsobem.
