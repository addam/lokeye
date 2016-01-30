# Dlouhodobý plán

* Lokální optimalizací najít pozici hlavy z nějaké odhadnuté pozice
* Přesně určit posuv zorniček oproti přímému pohledu
* Zkusit nafitovat na referenční data (pozice hlavy bude potřeba nahrubo zaklikat ručně)
(pak se uvidí)

# Menší nápady

* Porovnávání obrázků v pyramidě přepsat, aby na každé úrovni uvažovalo jen rozdíly oproti úrovni vyšší.
  Je to pak vlastně výpočet ve waveletové doméně.
  Mohlo by to pomoct proti nerovnoměrnému osvětlení, takže když například tvář přejde ze světla do stínu, budou se na ní pořád hledat stejné detailní struktury.
