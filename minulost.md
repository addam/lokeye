# Deník projektu

## Leden 2016 a dříve

Napsal jsem utilitu `label.cpp` na přesné označování zorniček nebo duhovek ve snímku.
Začne s kružnicí, jak ji označil uživatel, a lokálně ji vyoptimalizuje tak, aby okraj měl co největší kontrast.
Zdá se být dost dobrá pro samotný výpočet směru pohledu, ale musí být robustnější -- půlka duhovky bývá zakrytá.

Napsal jsem utilitu `transform.cpp` na otáčení v prostoru fotkou, ke které máme hloubkovou mapu.
Je pomalá a těžko se ovládá, ale slouží.

Zkusil jsem vyrábět hloubkovou mapu tak, že uživatel označí elipsu a ta se vyplní bublinou.
Tvaru hlavy to odpovídá jen zhruba, zvlášť v oblasti nosu jsou obrovské rozdíly.

Doplnil jsem funkci na porovnávání otočené fotky oproti referenční.
Porovnávání pixel-pixel moc nefunguje, ale smysluplné výsledky má, když se stejný přístup zopakuje na zmenšené verze fotky, jakoby v pyramidě.
Zkusil jsem taky fitování barevné transformace maticí 3x4 (světlost a kontrast ve všech kanálech RGB), ale to dělá spíš nesmysly.
Vyvážení barev totiž bývá stálé, mění se nanejvýš expozice.
Mám podezření, že velké problémy bude dělat osvětlení.

Vymodeloval jsem svoji hlavu z referenčních fotek a pomocí motion trackingu v Blenderu.
Doladění pozice hlavy se teď ručně dá udělat docela přesné, až na rozdíly výrazu ve tváři.

Vysvětlil jsem počítači, jak určit gradient chybové funkce podle šesti parametrů shodného zobrazení.
Napsal jsem kód, který hledá lokální minimum chybové funkce metodou největšího spádu se setrvačností.
Do určité míry to dovede hlavu umístit, ale není to (zatím?) dost dobré.

## Březen 2016

Přečetl jsem články:

### Biased manifold embedding: A framework for person-independent head pose estimation
  _Balasubramanian, Vineeth Nallure, Jieping Ye, and Sethuraman Panchanathan in IEEE CVPR 2007._
  Autoři navrhují docela jednoduchý způsob, jak upravit některé fitovací algoritmy, aby z nich šlo později získat přesná čísla -- v tomto případě úhel natočení hlavy. Jestli použiju nějaký podobný přístup, měl bych si článek ještě přečíst jednou pečlivěji.
  
### Real time head pose estimation with random regression forests
  _Fanelli, Gabriele, Juergen Gall, and Luc Van Gool in IEEE CVPR 2011._
  Článek se zabývá zpracováním dat z hloubkové kamery (jako je Kinect), a navíc získané rozlišení úhlů je asi deset stupňů, takže je docela nezajímavý. Chytré mi přijde generování trénovacích dat pomocí 3D modelu hlavy, takže je úhel bezchybně známý. 3D model pochází z práce [Paysan, Knothe, Amberg, Romdhani, Vetter: A 3d face model for pose and illumination invariant face recognition]

### A probabilistic framework for joint head tracking and pose estimation
  _Ba, Sileye O., and Jean-Marc Odobez in IEEE ICPR 2004._
  Cílem autorů je, aby jediný algoritmus odhadoval ve videu všech šest stupňů volnosti hlavy -- což mi připadá docela samozřejmé. Navrhovaný přístup vypadá spíš jako výpočet náhodným střílením a hrubou silou zabalený do pár pravděpodobnostních vzorečků. Nevím, jakého úhlového rozlišení dovede ten výpočet dosáhnout, ale ukázkové výpočty v článku rozlišují do přihrádek po 22,5 stupních a to naprosto nestačí.

### Head pose estimation for driver assistance systems: A robust algorithm and experimental evaluation
  _Murphy-Chutorian, Erik, Anup Doshi, and Mohan Manubhai Trivedi in IEEE ITSC 2007._
  Osvědčený přístup: histogram orientovaných gradientů a support vector machine. Hezká je péče o poctivá testovací data: pozici hlavy měřili v autě za jízdy soupravou několika kamer průmyslově vyrobených pro ten účel, a to ve dne i v noci. Za zmínku stojí taky, že noční výsledky jsou lepší -- díky IR přísvitu. Výpočet dosahuje chyby asi 5---10 stupňů.

### Head pose estimation on low resolution images
  _Gourier, Nicolas, et al. in Multimodal Technologies for Perception of Humans. Springer Berlin Heidelberg, 2006. 270-280._
  Používají přímo auto-asociativní neuronové sítě; vstup je zmenšený a oříznutý tak, aby obsahoval jen hlavu a měl 24x30 pixelů. Každá síť odpovídá jednomu konkrétnímu úhlu, takže rozlišení není pro nás zajímavé. Hezké je srovnání se schopností lidí rozpoznávat natočení hlavy: podle všeho je podobně chabé jako schopnosti tohoto algoritmu, totiž s chybou asi 10 stupňů. Autoři krom toho navrhují použít dva nezávislé výpočty pro úhel ve dvou osách, protože jen mírně uškodí výsledku a výrazně zrychlí výpočet (počet potřebných autoasociativních sítí se zmenší odmocninou).

### Head Pose Estimation
  _PENG, LAWYAN. Universiti Teknologi Malaysia, 2013._
  Disertace shrnující dosavadní poznatky o trasování hlavy. Pravdivá, ale špatná, navíc obrázky jsou vykradené z následujícího článku.
  
### Head pose estimation in computer vision: A survey
  _Murphy-Chutorian, Erik, and Mohan Manubhai Trivedi in IEEE Pattern Analysis and Machine Intelligence 31.4 (2009): 607-626._
  Pečlivý rozbor možných přístupů k rozpoznávání natočení hlavy. Zvlášť mě inspiruje skupina Manifold Embedding, algoritmus Adaptive Appearance Model a hybridní modely. Nečetl jsem všechny podrobnosti, ale zjevně jde celkově o vydatně probádanou oblast. Užitečné jsou reference na algoritmy a testovací databáze.

### Head pose estimation using view based eigenspaces
  _Srinivasan, Sujith, and Kim L. Boyer in IEEE ?, 2002._
  Sympaticky jednoduché řešení založené na PCA zmenšených obrázků. Vstupní obrázek se pak promítne postupně do eigenspace každého trénovacího kandidáta. Namísto přímého vyčtení úhlů z kandidáta s nejlepší shodou navrhují autoři krok s využitím hloubkové mapy, který nechápu. Výsledky opravují Kalmanovým filtrem.

### Head pose estimation by nonlinear manifold learning
  _Raytchev, Bisser, Ikushi Yoda, and Katsuhiko Sakaue in IEEE ICPR 2004._
  Pěkný přístup pro případ, že je k dispozici dost trénovacích dat z různých úhlů. Bohužel se ten článek vyžívá ve stroze podané maticové aritmetice, takže jen matně chápu, co se děje. Základem je algoritmus Isomap, popsaný v následujícím článku; rozšíření zřejmě umožňuje přesně vyhodnotit parametry pro libovolná vstupní data.

### A global geometric framework for nonlinear dimensionality reduction
  _Tenenbaum, Joshua B., Vin De Silva, and John C. Langford in science 290.5500 (2000): 2319-2323._
  Původní článek o široce používaném algoritmu Isomap. Trénovacím datům stanovíme euklidovskou metriku -- potřebujeme tedy mít data označená smysluplnými parametry. Pak vybudujeme graf namátkovým spojováním blízkých bodů hranou a ten graf se pokusíme deformovat do prostoru menší dimenze tak, aby se vzdálenosti zachovaly. Pro moje použití to ale postrádá smysl, protože data tvoří v prostoru parametrů docela kompaktní oblast (kompaktní v lidském smyslu).
