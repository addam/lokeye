# Aktuální plán a TODO list

poslední změna: 31. května

## Hledání oblasti očí ve snímku
Máme referenční obrázek (barevný) s vyznačeným obdélníkem, co hledat. Iterativně upravujeme plošnou transformaci obdélníka tak, aby se co nejlíp shodoval s aktuálním snímkem kamery. Těsné okolí očí je z optimalizace vynechané, aby zorničky nerušily.

## Hledání očí ve snímku
Tušíme, kde oči jsou, a chceme je obě najít přesně. Výpočet už vlastně fungoval, tak stačí totéž zprovoznit pro video, a to nejlíp živé.

## Odhad směru pohledu
Máme nějaká trénovací data, známe nějaké naměřené parametry a chceme zjistit, kam se uživatel zrovna dívá. Pro jednoduchost můžeme předpokládat, že vztah mezi pozicí očí a středem zájmu na obrazovce je dvourozměrná projektivita. Trénovací data si program získá tak, že uživateli postupně na různých místech ukazuje fixační značku a sbírá data, dokud nejde přesvědčivě nafitovat zobrazení.


## Přesný a úsporný výpočet
Program doteď derivoval obrázek pomocí Sobelova filtru tak, jak ho nabízí knihovna OpenCV. Nešetrné na tom bylo, že se zpracuje vždycky celý snímek; stačí přitom jenom malá oblast obličeje, a druhé derivace pak stačí v ještě menší oblasti kolem očí. Nepřesné je, že Sobelův filtr zabírá 3x3 okolí každého pixelu a detaily v obrázku se tak možná ztratí.

## Nástřel pomocí Houghovy transformace
Je možné, že výpočet je akorát příliš nestabilní: vážně to je tak, že zornička každou chvíli uskočí někam stranou a zůstane tak. Navíc tomu škodí, že při kalibraci je pohyb oka dost velký, někdy přes celý monitor. Jako první krok by se tedy měla najít kružnice se známým průměrem, a teprv odtamtud spustit lokální optimalizaci.

Potud hotovo.

## Výpočet v pyramidě
Hledání oblasti očí je náramně pomalé a nestabilní, když se pouští na obrázek v plné velikosti; hledání očí naopak potřebuje co nejlepší rozlišení. Proto by bylo určitě rozumné hledat oblast očí v obrazové pyramidě, zatímco oči stačí hledat jenom v nejpodrobnější kvalitě (moc se nehýbou).
