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
