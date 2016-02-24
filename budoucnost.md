# Dlouhodobý plán

únor:
    * ověřit, že pozice hlavy jde hrubou silou najít: že zvolená chybová funkce má jedno dost výrazné globální minimum, a to je řešení
    * rozvrhnout obecně text diplomky do kapitol
    * popsat svůj postup hledání pozice hlavy
    * najít a přečíst co nejvíc článků o přesném hledání hlavy
březen:
    * odladit hledání očí, když je pozice hlavy už dobře určená
    * zprovoznit výpočet směru pohledu pro dobře známé umístění kamery, monitoru atd. -- to znamená, navrhnout funkci, která to popisuje, a opatrně jí nafitovat na data
    * napsat větší část teoretické kapitoly
duben:
    * tipnutí tvaru hlavy z ničeho, klidně s použitím nějaké databáze hlav, nebo napasováním modelu na nalezené části obličeje
    * doladění tvaru hlavy pro přesnější výsledky, pokud je pozice hlavy už nějak nahrubo známá
    * dokončit (obsahově) teoretickou kapitolu
květen:
    * ladění všech výpočtů na rychlost
    * úklid kódu
    * dokončit (obsahově) implementační kapitolu
červen:
    * dokončit text tak, aby se dal číst, doplnit obrázky
    * připravit doprovodná data (~CD, tradičně)
červenec:
    * rezerva

# Menší nápady

* Porovnávání obrázků v pyramidě přepsat, aby na každé úrovni uvažovalo jen rozdíly oproti úrovni vyšší.
  Mohlo by to pomoct proti nerovnoměrnému osvětlení, takže když například tvář přejde ze světla do stínu, budou se na ní pořád hledat stejné detailní struktury.
