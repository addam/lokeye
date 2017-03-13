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

 * Biased manifold embedding: A framework for person-independent head pose estimation
  _Balasubramanian, Vineeth Nallure, Jieping Ye, and Sethuraman Panchanathan in IEEE CVPR 2007._
  Autoři navrhují docela jednoduchý způsob, jak upravit některé fitovací algoritmy, aby z nich šlo později získat přesná čísla -- v tomto případě úhel natočení hlavy. Jestli použiju nějaký podobný přístup, měl bych si článek ještě přečíst jednou pečlivěji.
 * Real time head pose estimation with random regression forests
  _Fanelli, Gabriele, Juergen Gall, and Luc Van Gool in IEEE CVPR 2011._
  Článek se zabývá zpracováním dat z hloubkové kamery (jako je Kinect), a navíc získané rozlišení úhlů je asi deset stupňů, takže je docela nezajímavý. Chytré mi přijde generování trénovacích dat pomocí 3D modelu hlavy, takže je úhel bezchybně známý. 3D model pochází z práce [Paysan, Knothe, Amberg, Romdhani, Vetter: A 3d face model for pose and illumination invariant face recognition]
 * A probabilistic framework for joint head tracking and pose estimation
  _Ba, Sileye O., and Jean-Marc Odobez in IEEE ICPR 2004._
  Cílem autorů je, aby jediný algoritmus odhadoval ve videu všech šest stupňů volnosti hlavy -- což mi připadá docela samozřejmé. Navrhovaný přístup vypadá spíš jako výpočet náhodným střílením a hrubou silou zabalený do pár pravděpodobnostních vzorečků. Nevím, jakého úhlového rozlišení dovede ten výpočet dosáhnout, ale ukázkové výpočty v článku rozlišují do přihrádek po 22,5 stupních a to naprosto nestačí.
 * Head pose estimation for driver assistance systems: A robust algorithm and experimental evaluation
  _Murphy-Chutorian, Erik, Anup Doshi, and Mohan Manubhai Trivedi in IEEE ITSC 2007._
  Osvědčený přístup: histogram orientovaných gradientů a support vector machine. Hezká je péče o poctivá testovací data: pozici hlavy měřili v autě za jízdy soupravou několika kamer průmyslově vyrobených pro ten účel, a to ve dne i v noci. Za zmínku stojí taky, že noční výsledky jsou lepší -- díky IR přísvitu. Výpočet dosahuje chyby asi 5---10 stupňů.
 * Head pose estimation on low resolution images
  _Gourier, Nicolas, et al. in Multimodal Technologies for Perception of Humans. Springer Berlin Heidelberg, 2006. 270-280._
  Používají přímo auto-asociativní neuronové sítě; vstup je zmenšený a oříznutý tak, aby obsahoval jen hlavu a měl 24x30 pixelů. Každá síť odpovídá jednomu konkrétnímu úhlu, takže rozlišení není pro nás zajímavé. Hezké je srovnání se schopností lidí rozpoznávat natočení hlavy: podle všeho je podobně chabé jako schopnosti tohoto algoritmu, totiž s chybou asi 10 stupňů. Autoři krom toho navrhují použít dva nezávislé výpočty pro úhel ve dvou osách, protože jen mírně uškodí výsledku a výrazně zrychlí výpočet (počet potřebných autoasociativních sítí se zmenší odmocninou).
 * Head Pose Estimation
  _PENG, LAWYAN. Universiti Teknologi Malaysia, 2013._
  Disertace shrnující dosavadní poznatky o trasování hlavy. Pravdivá, ale špatná, navíc obrázky jsou vykradené z následujícího článku.
 * Head pose estimation in computer vision: A survey
  _Murphy-Chutorian, Erik, and Mohan Manubhai Trivedi in IEEE Pattern Analysis and Machine Intelligence 31.4 (2009): 607-626._
  Pečlivý rozbor možných přístupů k rozpoznávání natočení hlavy. Zvlášť mě inspiruje skupina Manifold Embedding, algoritmus Adaptive Appearance Model a hybridní modely. Nečetl jsem všechny podrobnosti, ale zjevně jde celkově o vydatně probádanou oblast. Užitečné jsou reference na algoritmy a testovací databáze.
 * Head pose estimation using view based eigenspaces
  _Srinivasan, Sujith, and Kim L. Boyer in IEEE ?, 2002._
  Sympaticky jednoduché řešení založené na PCA zmenšených obrázků. Z trénovacích dat se vybudují eigenspaces podle natočení, napříč osobami. Vstupní obrázek se pak promítne do každého takového eigenspace. Namísto věštění z koeficientů se úhel vyčte z chyby každého (zhruba ze sedmi) eigenspace.
 * Pose estimation using 3D view-based eigenspaces
  _Morency, Sundberg, and Darrell in IEEE AMFG, 2003._
  Z trénovacích obrázků s hloubkovou mapou se vybuduje docela masivní model mnoha eigenspaces pro kvantizované úhly. Vstupní obrázek se pak promítne do vhodného z nich a pokud je k dispozici hloubková mapa, doladí se i ta. Výsledky jsou pěkné, ale jsem z toho postupu poněkud zmatený.
 * Head pose estimation by nonlinear manifold learning
  _Raytchev, Bisser, Ikushi Yoda, and Katsuhiko Sakaue in IEEE ICPR 2004._
  Pěkný přístup pro případ, že je k dispozici dost trénovacích dat z různých úhlů. Bohužel se ten článek vyžívá ve stroze podané maticové aritmetice, takže jen matně chápu, co se děje. Základem je algoritmus Isomap, popsaný v následujícím článku; rozšíření zřejmě umožňuje přesně vyhodnotit parametry pro libovolná vstupní data.
 * A global geometric framework for nonlinear dimensionality reduction
  _Tenenbaum, Joshua B., Vin De Silva, and John C. Langford in science 290.5500 (2000): 2319-2323._
  Původní článek o široce používaném algoritmu Isomap. Trénovacím datům stanovíme euklidovskou metriku -- potřebujeme tedy mít data označená smysluplnými parametry. Pak vybudujeme graf namátkovým spojováním blízkých bodů hranou a ten graf se pokusíme deformovat do prostoru menší dimenze tak, aby se vzdálenosti zachovaly. Pro moje použití to ale postrádá smysl, protože data tvoří v prostoru parametrů docela kompaktní oblast (kompaktní v lidském smyslu).

## Duben 2016

### Etapa od 29. března

Nápady:
 * zkusit v obrázku fitovat jen oblast kolem očí a to jen plošně, pro co nejjednodušší výpočet

Napsal jsem program _fit_eyes_: grafický nástroj na živé sledování obličeje a očí ve videu z webkamery. Posuv a rotace velmi dobře stačí pro sledování obličeje ve videu, ale otáčením hlavy doleva a doprava se výsledky znatelně rozjedou od správných hodnot.

### Etapa od 12. dubna

Přečíst články:
 * In the Eye of the Beholder: A Survey of Models for Eyes and Gaze.
   _Hansen, Ji in IEEE PAMI 2010._
   Velmi pěkný přehled. Dále prozkoumat: [166] fitování víček parabolou; [5][42][23][77] detekce mrknutí; [123] databáze testovacích dat; [95] přehled klávesnic na obrazovce; [140] Listing's laws, Donder's law? ; [79] dobré výsledky. Dobré připomenutí, že zornička se může posunout oproti duhovce, kvůli lomu.
 * A Survey on Eye-Gaze Tracking Techniques.
   _Chennamma, Yuan in IJCSE 2012._
   Vyjmenovává vtipná hardwarová řešení jako kontaktní čočky se zrcátky, ale nic moc užitečného pro mě.
 * Techniques Used for Eye Gaze Interfaces and Survey.
   _Rashid, Mehmood, Ashraf in IJASR 2015._
   Nedočetl jsem, je to strašná angličtina a zřejmě neříká nic nového.
 * Subpixel Eye Gaze Tracking.
   _Zhu, Yang in ? 2012._
   Jednoduché fitování na koutky očí a na duhovku. Směr pohledu prý stačí fitovat jako lineární funkci, to by bylo milé.
 * Appearance-based Eye Gaze Estimation.
   _Tan, Kriegman, Ahuja in IEEE WACV 2002._
   Najdou několik nejbližších sousedů v databázi očí a interpolují. Důležité prý je zohlednit zároveň s normou rozdílu obrázků oka i vzdálenost v prostoru parametrů -- napříč nalezenými sousedy, aby tvořili souvislou oblast. Článek ale nepopisuje, jak se hledají oči a jestli odhadují natočení hlavy; tipuju, že hlava je při měření fixovaná.
 * Eye Gaze Estimation from a Single Image of One Eye.
   _Wang, Sung, Venkateswarlu in IEEE ICCV 2003._
   Autoři nemají úplně dobrý cit na rozlišení podstatných informací od šumu, a promítá se to ostatně i do samotné metody. Zorničku v obrázku nafitují elipsou a z tvaru elipsy pak přímo dopočítají, jak je oko natočené. Na uměle vytvořených datech to funguje, v praxi to vyžaduje kameru zacílenou přímo na oko a ani tak nejsou výsledky moc přesvědčivé. Zajímavá myšlenka je, že zorničku hledají jen pomocí vodorovné derivace, to znamená svislých kontur.
 * A Novel Gaze Estimation System With One Calibration Point.
   _Villanueva, Cabeza in IEEE SMC 2008._
   Elegantně se vyhýbají potřebě sledovat obličej: na oku najdou odlesk ze známého zdroje (LED dioda) a panenku, což už dává o natočení oka dost informace. Je působivé, že se jim daří zacházet s tak detailním obrazem. Aby to fungovalo, budují velmi poctivý model toho, jak oko vypadá: oční bulva a rohovka jsou dvě kulové plochy, sklivec má známý index lomu, žlutá skvrna je od optické osy oka odchýlená o asi 5 stupňů. Prozkoumat: Donder's law [21]
 * Resolution of Focus of Attention Using Gaze Direction Estimation and Saliency Computation.
   _Yücel, Salah in IEEE ? 2009._
   Okultismus s neuronovými sítěmi. Tvar hlavy považují za válec a jeho pozici i natočení najdou ve snímku pomocí KLT trackeru. Výsledných šest parametrů (a nic víc) dostane neuronová síť, která má vyvěštit, kam se uživatel dívá; navíc se bere v potaz, které části výhledu by uživatele mohly zajímat. Možná to v praxi funguje, ale jako přístup je to hrozné. Přesnost pro moje účely nestačí. Prozkoumat: směr pohledu z natočení hlay [12]
 * A Method of Gaze Direction Estimation Considering Head Posture.
   _Zhang, Wang, Xu, Cong in IJSPIPPR 2013._
   Roztomile naivní postup: najít oči a pusu pomocí klouzajícího okna Adaboost klasifikátoru a ze získaných vzdáleností přímo trigonometricky napočítat natočení hlavy. Pozici zorničky najdou Houghovou transformací s přesností na devět možných směrů.

Nápady:
 * jednoduchý, jak to jde (lineární) odhad, kam se uživatel kouká, a UI na kalibraci
 * program, co bude malovat terčík a trasu uživatelova pohledu
 * přepsat výpočet, aby běžel v pyramidě
 * celkově úspornější výpočet

### Etapa od 28. dubna

Výpočet funguje, ale nedává vůbec dobré výsledky a není jasné proč.
Návrh: namísto kalibrace náhodným střílením projít několik bodů na přímce a prohlédnout si, jaká data vycházejí v takovém případě.

Přepsal jsem datovou strukturu pro obrázek, aby byla trochu přehlednější a především, aby mohla pokrývat jen malý výřez snímku, když je to potřeba.

## Květen 2016

Přepsal jsem interpolaci obrázku a derivací prvního a druhého řádu. Bilineární interpolace obrázku je sama o sobě jednoduchá, teoreticky vzato ale vede k nespojité derivaci. Možností se nabízí několik:
 * Úplně to odignorovat, jako všichni ostatní. Derivace se počítají z okolních dvou pixelů (potažmo čtyř v případě dxdy) a interpolovat se nechají zase bilineárně.
 * Datům líp odpovídá, když je první derivace posunutá na půl cesty mezi příslušnými dvěma pixely, a obdobně dxdy derivace je ve středu příslušných čtyř pixelů. Z teoretického hlediska to pořád není správně: takhle interpolované derivace spojité budou, ale bilineárně interpolovaný obrázek není hladký.
 * Teoreticky správné by bylo obrázek považovat za spojitou funkci a její derivace napočítat analyticky. Nejlákavější je použít Lanczosovu funkci sinc(x) * sinc(x/2); trápí mě ale, že konstantní signál interpolovaný touhle funkcí nezůstane konstantní. Filtrů, které by tuhle podmínku splňovaly a zároveň byly hladké, ale zřejmě není prozkoumaných moc a v praxi se nepoužívají. Jedna možnost je spartánský (cos(x) + 1)/2; po normalizaci na rozsah (-1...1) se ale ukáže, že derivace ve skutečnosti jsou příliš vysoké, že filtr vyrábí hrany navíc.
Nevím, možná chci něco, co neexisuje. Prozatím si vystačím si bilineární interpolací posunutou o půlpixely (jsem přesvědčený, že na tom záleží). Tohle téma podle mě ale zaslouží v diplomce aspoň stranu textu.

Průzkum, jak správně vyslovovat "Hough", mě navedl na zábavnou diskuzi: https://groups.google.com/forum/#!topic/misc.misc/m31Idr1tWzM

Derivace obrázku jsem nakonec udělal tak, aby šla počítat vždycky jenom v jednom směru (to znamená, nikdy ne celý gradient) a díky tomu aby byla správně posunutá. Předtím jsem se trochu zapeklil v tom, jak se má první a druhá derivace správně posouvat o půlpixely; je v tom systém, ale špatně se reprezentuje v počítači, protože se výsledky liší i ve svých rozměrech, o jeden pixel plus mínus.

Doplnil jsem inicializaci oka Houghovou transformací (kružnice se známým poloměrem). Když uživatel nehýbe hlavou, sledování očí teď dává výsledky přesné tak na 50 až 100 pixelů, podle toho, jak kalibrace vyjde. Zjevně to ale funguje. Otáčení hlavou má na výsledky předvidatelný, ale naprosto ničivý vliv.

Na letošním Siggraph byl podrobný tracker hlavy (včetně výrazu tváře) a očí: https://youtu.be/9O95A-3Ocbw (-> přečíst).

## Červen 2016

Přepsal jsem hledání obličeje tak, aby jednou prošlo obrazovou pyramidu. Zásadní zjištění je, že výpočet brutálně divergoval, když gradient nebyl správně posunutý o půlpixely; po šestnáctinásobném zmenšení znamená ten půlpixel už v původním obrázku obrovský posuv. Program teď stíhá zpracovat asi 4 snímky za vteřinu.

Zkusil jsem pro optimalizaci použít knihovnu Ceres; měl jsem za to, že bude dávat přesnější výsledky bez námahy, takže by šla použít jako referenční výpočet pro různé budoucí triky. Námahy bylo docela dost, použití na obrazová data vyžaduje alokovat tři wrappery na pixel. Především ale přímočarý KLT tracker nefunguje, s pyramidou ani bez ní. Tenhle plán opouštím a rovnice si budu řešit sám.

Přepsal jsem výpočet směru pohledu (`Gaze`) tak, aby vstupní vektor byl čtyřrozměrný; další dvě složky jsou otočení hlavy. Není vcelku problém to zobecnit na větší počet.

Napsal jsem rozpoznávání eigenfaces. Ukázkový program `test_eigenface` sleduje hlavu a buďto z videa počítá kovarianci, anebo vypisuje projekci na dvě principal components.

## Červenec 2016

Zabudoval jsem otočení hlavy podle eigenfaces do výpočtu homografie. Výpočet potom ale přestane fungovat, a to i v případě, že je hlava nehybná. Po několika šarlatánských pokusech se samotným DLT algoritmem (ne, ten je zřejmě v pořádku) jsem dospěl k závěru, že problém je v RANSAC algoritmu. Výsledky z eigenfaces mají příliš nízký poměr signálu k šumu, a každý minimální vzorek měření (v případě téhle homografie je to 7 trénovacích párů) je nevyhnutelně přeučený. Problém ale víceméně přetrvává i když spustím RANSAC se vzorky třeba dvojnásobné velikosti, takže možná ten přístup s eigenfaces je celkově příliš vágní.

Nasnímal jsem obrázek obličeje při všech natočeních směrem k monitoru, v mřížce 16x9. Mezi principal components se sice dá najít několik zástupců pro dvě kolmé osy, ale nejsou to ty první dvě a je to zatížené obrovským množstvím šumu. Předat tolik (deset až dvacet) parametrů zároveň s nafitovanýma očima a zacházet s natočením hlavy implicitně to tedy rozhodně nedovoluje. Explicitní kalibrace natočení hlavy (aby uživatel natáčel hlavu do čtyř, možná devíti směrů) padá v úvahu, ale byla by asi krkolomná.

Doplnil jsem hledání celého obličeje o dohledání malé oblasti u kořene a u konce nosu. Do dalšího výpočtu jde posuv očí oproti první z nich a posuv jich obou oproti sobě. Výsledky nejsou sice už závislé na natočení hlavy tak přímočaře, jako byly dřív, ale pořád při pohybu hlavy výrazně ujíždějí; musím podrobně prozkoumat, co se děje. Sledování nosu má potíž, že nosní dírky při otáčení hlavy výrazně mění svůj promítnutý tvar. Uvažuju kvůli tomu změnit transformaci na obecnou afinní, což by prospělo asi i zbytku obličeje.

Úvodní nastavení teď vyžaduje pět pečlivých tahů myší a kalibrace trvá zhruba minutu. Možná by stačilo ukládat výsledky od minula a obličej zkusit na začátku najít obyčejným sliding window. Kvůli změnám osvětlení ale může být potřeba normalizace světlosti a kontrastu -- možná by přišla vhod beztak. Proti změnám světlosti za běhu programu by mohlo prospět pozvolna měnit referenční obrázek, s poločasem rozpadu aspoň minutu.

Dopsal jsem afinní transformaci. Funguje a kód je dokonce o chlup hezčí.

Několik dnů jsem strávil zkoumáním, co se stane matici 2D -> 2D homografie, když jedním ze čtyř bodů pohnu. Ty objevy mě dost překvapily. Jednak, pro homografii čtyř bodů stačí vyřešit jednu lineární soustavu 3x3 -- a ne 8x8, jak by to udělalo DLT. Ten fígl je, převést body do vhodné kanonické konfigurace a zpátky. Druhak, výsledek je lineární vzhledem k oběma souřadnicím jednoho zvoleného bodu (homogenní matice 3x3, až na škálu, pochopitelně). Zobecnění na zbylé tři body se dá zařídit maticí, která je zpermutuje. Na Internetu ani v _Multiple View Geometry_ jsem o tomhle nenašel zmínku.

## Srpen 2016

Docela náhodou jsem objevil program eViacam, kterýžto ovládá myš podle pohybu hlavy. Funguje spolehlivě a hezky i bez nastavování. Zdrojový kód prozrazuje, že celý výpočet je pár funkcí z OpenCV: najít v obrázku hlavu, na ní najít pár významných bodů, ty trackovat KLT algoritmem a zprůměrovat jejich směr. Otáčení a posouvání hlavy má tedy stejný vliv, mně se to nehodí. Je to ale hezká utilita.

## Prosinec 2016

Napsal jsem barycentrickou a perspektivní transformaci jako dvě další alternativy.
Zkoušení programu naživo mě přesvědčilo, že výpočet selhává hlavně na hledání duhovky / zorničky: to často uletí úplně mimo oko a jen pomalu se vrací.

## Únor 2017

Napsal jsem ukázkové programy k odzkoušení transformací na statickém záběru.
Všechny fungují bezchybně, ale hodně pomalu.

Napsal jsem program pod Windows, který nahraje člověka u počítače a zaznamená jeho směr pohledu z Tobii EyeX, snímek po snímku.

### Etapa od 20. února

Nápady ze schůzky:

 * měl bych odzkoušet neuronové sítě. Tak jako tak budu potřebovat velký dataset očí -> opatřit ho.
 * Odzkoušet, jaké výsledky vrací výpočet při sledování známého fixního bodu. Možná utíká výpočet, krom toho trochu utíkají oči. Jaké výsledky pak dává Tobii?
 * Pročíst články o hledání očí obecně a dále eigeneyes, neuronové sítě, Stefanos Zafeiriou. 

Stáhnul jsem datasety:

* Labeled pupils in the wild (2015): http://www.hci.iis.u-tokyo.ac.jp/~cvs/datasets/ : sada 66 videí různých lidí s detailní kamerou na oko. Každý snímek má označené obrazové těžiště panenky (proto nepřesně). Kamera byla k obličeji napevno přidělaná, většinou hodně zblízka a z boku -- to je dost jiná situace, asi nepůjde použít.
* Appearance-based Gaze Estimation in the Wild (Zhang, Sugano, Fritz, Bulling, 2015), odtud MPIIGaze dataset: 15 lidí v průběhu měsíce požádala aplikace na laptopu, aby sledovali několik křížků na monitoru. Z webkamery pořízená spousta fotek v dost rozmanitých (a mnohdy mizerných) podmínkách. Článek srovnává různé techniky určování směru pohledu s vlastním přístupem pomocí konvolučních sítí -- ty zjevně fungují skvěle.
* Synthes Eyes (2015): https://www.cl.cam.ac.uk/research/rainbow/projects/syntheseyes/ : uměle renderované oči s náhodnými parametry. Celkem 12 modelů levého oka a blízkého okolí, každý vyrenderovaný tisíckrát v různém osvětlení, různém natočení kamery a směru pohledu oka (včetně přivírání víčka). Všechny parametry jsou v datasetu uložené, je to dokonale přesné. Některé snímky jsou v úplné tmě, na jiných je z panenky vidět jenom růžek.
* Fotky z instagramu (aktuální k 20. 2. 2017): https://www.instagram.com/explore/tags/selfie/ : ručně jsem pořídil pomocí downloaderu https://www.instadownloader.net/hashtag a odtud vybral 150 fotek s jednoznačně viditelnýma očima a bez výraznějších filtrů. Každá fotka je jiný člověk, zahrnuje to všechny možné národnosti a světelné podmínky. Pochopitelně, zorničky ani směr pohledu nejsou nijak označené.
* BioID database: https://www.bioid.com/About/BioID-Face-Database : ručně označené (první dva landmarky jsou pravá a levá zornička) fotky v umělých podmínkách. Černobílé obrázky ~320x240. Některé obrázky jsou se zavřenýma očima, to je absurdní.
* seznam dalších datasetů: http://homepages.inf.ed.ac.uk/rbf/CVonline/Imagedbase.htm#face (všechny zmíněné tam jsou, ten seznam je asi aktuální)
* další seznam: https://sites.google.com/a/nd.edu/public-cvrl/data-sets

Přečetl jsem články:

* Unsupervised Eye Pupil Localization through Differential Geometry and Local Self-Similarity Matching (Leo, Cazzato, Marco, Distante 2014): součet kružnicové Hough transformace a kros-korelace při různém otočení (míra rotační symetrie). Oči z obličejů vyřízli nahrubo adaboostem (Matlab CV). Asi to funguje spolehlivě, kros-korelace stojí za vyzkoušení. Přesnost je nevyhnutelně na úrovni pixelů a ten přístup je zbytečně globální -- pro stabilitu se navíc kombinuje ze všech úrovní pyramidy.
* Eye pupil localization with an ensemble of randomized trees (Makus et al 2013): rozhodovací stromy na výřez oka. Listy jsou konstantní, takže přesnost tak akorát stačí k tomu, aby se strefila někam dovnitř zorničky. Tohle rozhodně nestačí, a obecně mi rozhodovací stromy pro tuhle úlohu nepřijdou dobré, protože špatně zacházejí s posuvem v obrázku.
* program Pupil (Kassner, Patera, Bulling): fungující program určený pro do-it-yourself eye trackery s kamerou namontovanou k hlavě. Zorničku hledá fitováním elipsy na tmavou oblast v obrázku, přepočet na souřadnice v obrázku zařídí nafitovaný polynom.
* Cross spread pupil tracking technique (Wolski, Mantiuk 2016): minimalistické zaměřování zorničky vrháním paprsku z jejího vnitřku. Ignoruje to zakrytí očními víčky, takže je to pro mě naprosto nevhodné.
* Realtime 3D Eye Gaze Animation Using a Single RGB Camera (Wang, Shi, Xia, Chai 2016): sledování celé hlavy a mimo jiné očí. 
Rozpoznává, jak se člověk tváří, a tedy je vůči tomu odolný.
Sledování očí je motivované, aby to v animaci vypadalo přesvědčivě.
Citují Tobii jakožto IR přístup, ale není k tomu žádný článek. Zmiň
Citují určování gaze z obrázku oka a předpočítaného natočení hlavy pomocí CNN: Sugano 2014, Wood 2015, Zhang 2015.
Model obličeje lineární parametrizací z Face Warehouse (Cao 2014): výraz a osoba jsou explicitní, parametry osoby se zafixují
Klasifikace pixelů kolem oka: obrázek normalizujeme k podobnostní transformaci a na greyscale patch pošleme rozhodovací les.
Zorničku tipnou pomocí Mean Shift na (dost chlupaté) pravděpodobnosti z klasifikace, potom si pomůžou Distance transformací (hodně pročištěných) dat z Cannyho detektoru, a nakonec to celé zabalí do MAP optimalizace s ohledem na předchozí snímek.
Detekce zavřených očí pomocí náhodného lesa.
Směr pohledu obou očí zároveň musí mít dost blízkého souseda v trénovací databázi, jinak se bere jako chyba.

* Face alignment at 3000 fps via regressing local binary features (Ren, Cao.. 2014)
Local Binary Pattern: srovnání (1-bit) středu pravidelného n-úhelníka s vrcholy, v každém pixelu
jak se budují ty stromy -- jsou to LBP, a když, tak z jakých pixelů?
co je to za hodnoty, které jdou do lineární regrese?

Seznam k přečtení:

* Handbook of iris recognition (Springer, 2016): kapitoly 8-10 se přímo zabývají hledáním oka. Mám stažené.
* Face Recognition in Adverse Conditions (IGI Global, 2014): sborník o hledání obličejů obecně, očím se věnuje sotva jedna kapitola. Celkově to je náramný zdroj referencí na články a datasety. Podařilo se mi stáhnout ze safaribooksonline.com.
* Visualizing and Understanding Convolutional Networks (Zeiler, Fergus 2013)
* Real-time high-fidelity facial performance capture (Cao, Bradley, Zhou, Beller 2015)
* Megvii: http://www.faceplusplus.com.cn

Myšlenky:

* Mean Shift na hledání zorničky vypadá šikovně. Když se vezme jako jeden blok spolu s klasifikací, vlastně má něco společného s mým iteračním schématem. Podstatné je ohodnotit všechny pixely uvnitř, a nepatlat se s derivacema. Mean shift pro obecný kernel nemá zaručenou konvergenci. Možná stojí za srovnání poctivý přístup s derivacema a pragmatické průměrování.

### Etapa od 7. března

Myšlenky ze schůzky:
* Nedali autoři MPIIGaze nějaké mezivýsledky ke stažení -- pozice očí, natočení hlavy?
* do příště natočit pár videí s dobrým osvětlením a vrátit Tobii
* implementovat +- algoritmus na hledání očí z [Wang, Shi... 2016] a případně si s ním pohrát
