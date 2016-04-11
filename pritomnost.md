# Aktuální plán a TODO list

poslední změna: 12. dubna

## Hledání oblasti očí ve snímku
Máme referenční obrázek (barevný) s vyznačeným obdélníkem, co hledat. Iterativně upravujeme plošnou transformaci obdélníka tak, aby se co nejlíp shodoval s aktuálním snímkem kamery. Těsné okolí očí je z optimalizace vynechané, aby zorničky nerušily.

## Hledání očí ve snímku
Tušíme, kde oči jsou, a chceme je obě najít přesně. Výpočet už vlastně fungoval, tak stačí totéž zprovoznit pro video, a to nejlíp živé.

Potud hotovo.

## Odhad směru pohledu
Máme nějaká trénovací data, známe nějaké naměřené parametry a chceme zjistit, kam se uživatel zrovna dívá. Znamená to navrhnout na papíře vhodnou funkci, tu za běhu programu nafitovat na trénovací data a pak ji prostě vyhodnotit. A nějak hezky namalovat na obrazovku.

## Výpočet v pyramidě
Hledání oblasti očí je náramně pomalé a nestabilní, když se pouští na obrázek v plné velikosti; hledání očí naopak potřebuje co nejlepší rozlišení. Proto by bylo určitě rozumné hledat oblast očí v obrazové pyramidě, zatímco oči stačí hledat jenom v nejpodrobnější kvalitě (moc se nehýbou).
