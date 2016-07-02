# Aktuální plán a TODO list

poslední změna: 2. července

## Optimalizaci nechat na knihovně Ceres
Ceres nabízí několik klasických algoritmů pro nelineární optimalizaci. Derivace si počítá sama, a to zčásti už během kompilace. Docela jistě prospěje přesnosti výpočtu, namísto metody největšího spádu s krokem pevné délky, jak ho používám teď.

Neprospěje. Výpočet nefunguje a kód je spíš ošklivější, než byl dřív, proto si optimalizaci zařídím sám.

## Zohlednit natočení hlavy
Můžeme sice předpokládat, že se uživatel dívá na monitor přímo, ale i malé otočení hlavy má na výsledky obrovský vliv. V neměnném směrovém osvětlení by mohlo stačit otáčení hlavy modelovat jako lineární kombinaci obrázků, na způsob eigenfaces. Rozlišení by mělo být dost hrubé, aby pohyb očí už nebyl vidět.

Potud hotovo, ale nefunguje pak fitování gaze.

Správnější přístup by byl odhadnout hloubkovou mapu a do modelu hlavy pak přidat ještě vliv optical flow. Při hrubém rozlišení by to mohlo být i docela stabilní. Řešily by se tak problémy s obroučkami brýlí, které akorát překážejí v oblasti u kořene nosu a nepředvídatelným způsobem čouhají do prostoru.

Robustní model natočení celého obličeje by zároveň měl poskytnout odhad toho, jak aktuálně vypadají vnitřní koutky očí. To může hodně prospět přesnosti výpočtu.

## Hledat cíleně vnitřní koutky očí
Porovnávání pohybu očí oproti celému obličeji je nesmírně nepřesné, když se uživatel začne divně tvářit. Z vlastního pozorování před zrcadlem vyplývá, že vnitřní koutky očí jsou docela pevně fixované na lebku (do textu diplomky opatřím podklady z učebnice anatomie). Aby výpočet zvládal prudké pohyby hlavy, hodí se ale celý obličej stejnak najít, jako první krok.

## Zohlednit víčka
Program hledá obrys duhovky (angl. limbus) jako kružnici s tmavým vnitřkem. Přitom, i když se uživatel dívá skoro přímo do kamery, je zhruba půlka obrysu zakrytá víčky, a to patrně zkresluje výsledek. Robustnější chybová funkce pro oko by tedy měla na obrysu určit i váhovou funkci, která bude na víčku nulová. Jedno řešení je víčka explicitně modelovat, což by bylo užitečné i pro hledání koutků očí. O něco jednodušší možná bude předpokládat, že obrys duhovky má celý stejnou barvu, tu nějakým způsobem robustně odhadnout a na jejím základě klasifikovat každý pixel jednotlivě.

## Deterministická kalibrace
Aby šlo výpočet předvádět opakovaně s jednou natočeným videem, musí být možné napevno zvoli sekvenci fixací, jaká se ukazuje během kalibrace. Problém je se synchronizací; proto by možná bylo nejlepší údaje o kalibrační sekvenci i s čísly snímku někam uložit. Týž program může uložit i video z kamery, v OpenCV to není problém.

##takový program už by měl bohatě stačit.
