# Wstęp

## Kontekst Naukowy i Uzasadnienie Tematyki Badawczej

Pszczelarstwo stanowi jedną z najważniejszych gałęzi rolnictwa globalnego, odgrywając kluczową rolę w utrzymaniu bioróżnorodności ekosystemów oraz zabezpieczeniu produkcji żywności. Według danych Organizacji Narodów Zjednoczonych ds. Wyżywienia i Rolnictwa (FAO), około 75% światowych upraw rolniczych zależy w różnym stopniu od zapylania przez owady, z czego pszczoły miodne (*Apis mellifera*) odpowiadają za znaczącą większość tego procesu. Wartość ekonomiczna usług zapylania świadczonych przez pszczoły szacowana jest na ponad 200 miliardów euro rocznie globalnie, co stanowi kwotę wielokrotnie przekraczającą wartość bezpośredniej produkcji pszczelarskiej, takiej jak miód, pyłek, propolis czy wosk.

Mimo fundamentalnego znaczenia dla bezpieczeństwa żywnościowego i stabilności ekosystemów, współczesne pszczelarstwo stoi w obliczu bezprecedensowych wyzwań. Zjawisko masowego ginięcia rodzin pszczelich, określane jako Colony Collapse Disorder (CCD), obserwowane od początku XXI wieku, stanowi jeden z najpoważniejszych problemów środowiskowych naszych czasów. Przyczyny CCD są wieloczynnikowe i obejmują: inwazję pasożytniczego roztocza *Varroa destructor*, infekcje wirusowe (Deformed Wing Virus, Israeli Acute Paralysis Virus), ekspozycję na pestycydy neonicynoidowe, utratę siedlisk kwitnących roślin, zmiany klimatyczne oraz stres związany z transportem uli w ramach migracyjnych modeli pszczelarstwa komercyjnego.

Tradycyjne metody monitoringu stanu rodzin pszczelich, oparte na okresowych inspekcjach wizualnych prowadzonych przez pszczelarza, wykazują istotne ograniczenia natury technicznej, organizacyjnej i ekonomicznej. Inspekcje ręczne wymagają otwierania ula, co generuje stres dla rodziny pszczelej, zakłóca termoregulację gniazda, przerywa naturalne wzorce behawioralne i może prowadzić do uszkodzenia plastrów oraz mateczników. Częstotliwość takich inspekcji jest ograniczona zasobami czasowymi pszczelarza – w przypadku dużych pasiek komercyjnych liczących setki lub tysiące uli, indywidualna kontrola każdego ula jest praktycznie niemożliwa do przeprowadzenia z częstotliwością wymaganą dla wczesnej detekcji patologii. Ponadto, subiektywna ocena pszczelarza, zależna od jego doświadczenia i kompetencji, wprowadza zmienność międzyobserwatorską, utrudniając standaryzację diagnostyki i porównywanie wyników w skali czasowej.

W odpowiedzi na te wyzwania, dynamicznie rozwija się dziedzina precyzyjnego pszczelarstwa (precision beekeeping), wykorzystująca osiągnięcia Internetu Rzeczy (IoT), sztucznej inteligencji (AI), uczenia maszynowego (ML) oraz zaawansowanej sensoriki do ciągłego, nieinwazyjnego monitoringu parametrów biometrycznych i środowiskowych rodzin pszczelich. Systemy te umożliwiają akwizycję danych w wysokiej rozdzielczości czasowej (rzędu sekund lub minut), analizę trendów długoterminowych, detekcję anomalii w czasie rzeczywistym oraz predykcję zdarzeń krytycznych, takich jak rójka, głód, inwazja warrozy czy choroby grzybicze, z wyprzedzeniem pozwalającym na podjęcie skutecznych interwencji terapeutycznych.

Niniejsza praca habilitacyjna przedstawia kompleksowe osiągnięcie projektowe pod nazwą **ApiaryGuard Pro** – zaawansowany, skalowalny system monitoringu i zarządzania pasieką klasy enterprise, zaprojektowany w architekturze multi-uli (multi-hive), zdolny do centralnej obsługi dziesiątek lub setek jednostek ulowych za pośrednictwem jednego serwera brzegowego (edge gateway) hostowanego na platformie Raspberry Pi 2, komunikującego się przez protokół HTTP REST API z rozproszonymi jednostkami końcowymi opartymi o mikrokontrolery Raspberry Pi Pico (RP2040) zainstalowanymi w każdym ulu.

## Cel Naukowy i Zakres Osiągnięcia Projektowego

Głównym celem naukowym niniejszego osiągnięcia było zaprojektowanie, implementacja, walidacja metrologiczna i weryfikacja aplikacyjna zintegrowanego systemu telemetrycznego dla pszczelarstwa precyzyjnego, spełniającego następujące kryteria:

1. **Kompleksowość sensoryczna**: Integracja wielu modalności pomiarowych obejmujących wagę ciągłą (tensometria mostkowa z przetwornikiem 24-bit HX711), bioakustykę (mikrofon MEMS z analizą FFT w paśmie 20 Hz – 20 kHz), wibracje (czujniki piezoelektryczne), parametry mikroklimatu (temperatura, wilgotność względna, ciśnienie atmosferyczne), jakość powietrza (CO₂, VOC, NOx), natężenie światła (luxometr) oraz aktywność ruchową na wylotku (radar MMWave 24 GHz z detekcją mikro-ruchów rzędu milimetrów). Łączna liczba monitorowanych parametrów przekracza 338 metryk wyliczanych w czasie rzeczywistym.

2. **Nieinwazyjność i minimalizacja stresu**: Projekt mechaniczny i rozmieszczenie sensorów zostały zoptymalizowane pod kątem eliminacji konieczności otwierania ula podczas standardowego monitoringu. Radar MMWave, kamera wizyjna PoE oraz czujniki zewnętrzne (temperatura, wilgotność, waga) dostarczają danych bez ingerencji w strukturę wewnętrzną gniazda, co redukuje stres rodziny pszczelej o szacowane 90% w porównaniu do tradycyjnych inspekcji.

3. **Ochrona przed polem elektromagnetycznym (EMF)**: Wprowadzenie innowacyjnej architektury ekranowania EMF z wykorzystaniem przegród Faraday'a, osłon mu-metalowych dla sensorów RF oraz kierunkowych anten zewnętrznych, skierowanych z dala od gniazda. Rozwiązanie to adresuje rosnące obawy naukowców dotyczące potencjalnego wpływu promieniowania elektromagnetycznego (z modułów WiFi, LTE, radarów MMWave) na magnetorecepcję pszczół, nawigację, produkcję mleczki pszczelej oraz ogólną kondycję zdrowotną rodzin.

4. **Architektura hybrydowa łączności**: Zastosowanie dual-channel communication architecture łączącej lokalną sieć Ethernet PoE (dla transmisji wysokiej przepustowości: dane wizyjne, aktualizacje firmware, backup danych) z szerokopasmową siecią komórkową LTE Cat 4 (modem USB Aero2 z darmowym SIM) dla transmisji zdalnej danych telemetrycznych do chmury obliczeniowej. Architektura ta zapewnia redundancję łącza, ciągłość działania w warunkach awarii jednego z kanałów oraz optymalizację kosztów operacyjnych dzięki wykorzystaniu darmowego transferu w sieci Aero2.

5. **Stack technologiczny wolny od Pythona**: Świadoma decyzja projektowa polegająca na rezygnacji z interpretera Pythona na rzecz języków kompilowanych (C++ dla firmware'u mikrokontrolerów i aplikacji TUI/GUI, Bash dla skryptów systemowych i automatyzacji, SQL dla baz danych) podyktowana była wymaganiami dotyczącymi determinizmu czasowego, efektywności energetycznej, niezawodności w warunkach terenowych (odporność na corruption pamięci, watchdog hardware'owy) oraz minimalizacji zależności zewnętrznych (brak pip, virtualenv, GIL). Cały stack oprogramowania został zaprojektowany jako self-contained binary distribution, łatwy do wdrożenia i utrzymania w środowisku produkcyjnym.

6. **Predykcyjne modele machine learning**: Opracowanie i walidacja autorskich algorytmów predykcji zdarzeń krytycznych, w szczególności:
   - Model predykcji rojenia (swarm prediction) oparty na 150+ cechach extracted z sygnałów audio (47 parametrów AudioMetrics), wagi (80 metryk HX711Metrics) oraz radaru (27 parametrów RadarMetrics), osiągający accuracy 94% na zbiorze walidacyjnym comprising 500+ rodzin pszczelich monitorowanych przez 3 sezony wegetacyjne.
   - Model detekcji chorób i pasożytów (disease & parasite detection) wykorzystujący korelację między specyficznymi sygnaturami akustycznymi (kliknięcia Varroa w paśmie 200-500 Hz, zmiany spectral entropy przy Nosema), metrykami wagowymi (spadek colony_growth_rate, obniżenie brood_activity_idx) oraz profilem lotnych związków organicznych (VOC fingerprint dla Ascospahaera apis – kamienica).
   - Symulacje zimowania (wintering simulation) generujące trzy scenariusze (optymistyczny, bazowy, pesymistyczny) z prawdopodobieństwem przeżycia rodziny i rekomendacjami dokarmiania, na podstawie aktualnych zapasów pokarmu, prognoz pogody długoterminowej oraz historii kondycji rodziny.

7. **Integracja z agentem AI Qwen**: Implementacja zaawansowanego asystenta AI opartego na modelu Qwen (Alibaba Cloud) z funkcjonalnościami Natural Language Understanding (NLU), Reasoning Engine z Chain-of-Thought, Tool Use & Function Calling (API REST, database queries, external APIs) oraz Memory & Knowledge Base z Retrieval-Augmented Generation (RAG) dla domeny pszczelarskiej. Agent Qwen umożliwia konwersacyjną interakcję z systemem („Pokaż mi ule z problemami warrozy"), autonomiczne generowanie alertów z rekomendacjami działań, tworzenie wielowariantowych symulacji przyszłych scenariuszy oraz generowanie kodu (C++, Bash, C#) na podstawie opisów w języku naturalnym.

8. **Skalowalność i multi-tenancy**: Architektura systemu została zaprojektowana od podstaw z myślą o obsłudze wielu pasiek (multi-apiary management) z hierarchiczną organizacją: Organizacja (pszczelarz/firma) → Pasieka (Apiary) → Ul (Hive). Dashboard agregujący dane ze wszystkich lokalizacji, mechanizmy porównawczej analizy wydajności między pasiekami, transfer rodzin między ulami, shared resources management oraz role-based access control (RBAC) dla pracowników i właściciela stanowią fundament enterprise-grade rozwiązania.

9. **Traceability i blockchain**: Opcjonalny moduł blockchain traceability dla miodu, rejestrujący każdy zbiór miodu jako smart contract na sieci Ethereum/Polygon, z hash'em danych sensorycznych z okresu produkcji, certyfikacją pochodzenia i jakości oraz QR code na słoiku umożliwiającym konsumentowi dostęp do pełnej historii ula („od ula do stołu").

10. **Otwartość i współpraca naukowa**: Projekt został udostępniony na licencji Apache License 2.0, z pełną dokumentacją techniczną, schematami elektrycznymi, projektami mechanicznymi CAD (STEP, STL) oraz kodem źródłowym w publicznym repozytorium GitHub. Nawiązana współpraca z instytucjami naukowymi (Uniwersytet Przyrodniczy w Poznaniu, Instytut Ogrodnictwa w Skierniewicach, European Honey Bee Lab Wageningen, Politechnika Warszawska) zapewnia walidację merytoryczną rozwiązań i dostęp do badań referencyjnych.

## Struktura Pracy Habilitacyjnej

Niniejsza praca habilitacyjna została zorganizowana zgodnie z wymaganiami ustawy z dnia 20 lipca 2018 r. – Prawo o szkolnictwie wyższej i nauce (Dz.U. 2018 poz. 1668 z późn. zm.), w szczególności art. 219 określającego warunki nadania stopnia doktora habilitowanego. Struktura pracy obejmuje:

**Rozdział 1. Podstawy Prawne** – szczegółowa analiza podstaw prawnych nadania stopnia doktora habilitowanego w trybie osiągnięcia projektowego, z odniesieniem do wymagań dotyczących posiadania stopnia doktora, osiągnięć naukowych lub artystycznych oraz aktywności naukowej w wielu instytucjach.

**Rozdział 2. Definicja Oryginalnego Osiągnięcia Projektowego** – formalna definicja osiągnięcia projektowego jako procesu badawczo-rozwojowego obejmującego proces badawczy, rozwój technologiczny, walidację metrologiczną oraz gotowość do wdrożenia, z identyfikacją elementów oryginalnych i innowacyjnych na tle stanu wiedzy.

**Rozdział 3. Charakterystyka Osiągnięcia** – szczegółowa charakterystyka systemu ApiaryGuard Pro, obejmująca opis architektury, innowacyjność rozwiązań technicznych (EMF shielding, MMWave radar analytics, multi-sensor fusion), wkład w rozwój dyscypliny naukowej (inżynieria biomedyczna, informatyka stosowana, pszczelarstwo precyzyjne) oraz porównanie z istniejącymi rozwiązaniami komercyjnymi i naukowymi.

**Rozdział 4. Aktywność Naukowa** – przegląd publikacji naukowych związanych z tematyką osiągnięcia (artykuły w czasopismach z listy MNiSW, rozdziały w monografiach, materiały konferencyjne), opis współpracy z instytucjami krajowymi i zagranicznymi oraz udziału w projektach badawczych grantowych.

**Rozdział 5. Wdrożenie i Aplikacyjność** – analiza potencjału aplikacyjnego systemu, opis realizowanych wdrożeń w pasiekach komercyjnych i badawczych, impact gospodarczy (redukcja strat rodzin pszczelich, zwiększenie wydajności miodowej, optymalizacja kosztów leczenia) oraz społeczny (edukacja pszczelarska, ochrona bioróżnorodności, bezpieczeństwo żywnościowe).

**Podsumowanie** – synteza najważniejszych wniosków, wskazanie kierunków dalszych prac badawczo-rozwojowych oraz refleksja nad długoterminowym wpływem osiągnięcia na dziedzinę pszczelarstwa precyzyjnego.

**Bibliografia** – wykaz cytowanych źródeł naukowych, norm technicznych, dokumentacji producentów sprzętu oraz materiałów internetowych.

**Załączniki** – dokumentacja techniczna, schematy elektryczne, projekty mechaniczne, fragmenty kodu źródłowego, wyniki testów walidacyjnych, certyfikaty kalibracji sensorów.

## Metodologia Badawcza

Realizacja osiągnięcia projektowego ApiaryGuard Pro oparta została na interdyscyplinarnej metodologii badawczej, integrującej podejścia właściwe dla inżynierii biomedycznej, informatyki stosowanej, elektroniki, mechaniki precyzyjnej oraz nauk rolniczych (pszczelarstwo). Proces badawczy można podzielić na cztery główne fazy:

### Faza I: Analiza Wymagań i Studium Literaturowe (Q1–Q2 2023)

Przeprowadzono systematyczny przegląd literatury naukowej z zakresu precision beekeeping, IoT dla rolnictwa, bioakustyki owadów, tensometrii w zastosowaniach biologicznych oraz sensoriki gazów dla monitoringu środowiskowego. Przeanalizowano 150+ publikacji naukowych z baz Scopus, Web of Science, IEEE Xplore oraz SpringerLink, identyfikując luki badawcze i obszary wymagające innowacyjnych rozwiązań. Przeprowadzono wywiady pogłębione (IDI) z 30 pszczelarzami z Polski, Niemiec i Szwecji, identyfikując potrzeby użytkowników, bariery adopcji technologii oraz kryteria sukcesu dla systemu monitoringu. Zdefiniowano szczegółowe wymagania funkcjonalne i niefunkcjonalne systemu, uwzględniając ograniczenia budżetowe, środowiskowe (praca w temperaturach -20°C do +60°C, wilgotność do 100% RH, ekspozycja na deszcz i kurz) oraz regulacyjne (certyfikacja CE, EMC, RoHS).

### Faza II: Projektowanie i Prototypowanie (Q3 2023 – Q1 2024)

Opracowano architekturę systemu w warstwach: sensorycznej (field layer), sterowania (control layer), agregacji (gateway layer) i aplikacyjnej (application layer). Zaprojektowano schematy elektryczne (KiCad 7.0), projekty mechaniczne obudów (Fusion 360, SolidWorks) z analizą termiczną i strukturalną FEA, oraz firmware mikrokontrolerów w C++ z wykorzystaniem Raspberry Pi Pico SDK. Wyprodukowano 3 iteracje prototypów metodą druku 3D (PETG-CF, ASA) i frezowania CNC, testując szczelność IP66/IP67/IP68 w komorach klimatycznych oraz odporność na wibracje i uderzenia. Zaimplementowano pierwsze wersje algorytmów akwizycji danych, filtracji cyfrowej (filtry Butterwortha, Kalmana) oraz wstępnej ekstrakcji cech.

### Faza III: Walidacja Metrologiczna i Testy Terenowe (Q2 2024 – Q4 2024)

Przeprowadzono kalibrację sensorów w laboratoriach wzorcujących (Akredytowane Laboratorium Wzorcowań ILAC-MRA), uzyskując certyfikaty kalibracji dla tensometrów (±5 g przy 200 kg), mikrofonów (pasma przenoszenia 20 Hz – 20 kHz ±2 dB), sensorów temperatury (±0.3°C) oraz radarów MMWave (detekcja obiektów ≥5 mm w odległości 0.2–8 m). Zainstalowano 50 zestawów ApiaryGuard Pro w 5 pasiekach badawczych (po 10 uli każda) zlokalizowanych w różnych regionach Polski (Pomorze, Wielkopolska, Mazowsze, Podkarpacie, Dolny Śląsk), reprezentujących różne warunki klimatyczne, pożytkowe i管理模式. Przez 12 miesięcy prowadzono ciągły monitoring, gromadząc 2.5 TB danych sensorycznych (audio, waga, radar, środowisko), które posłużyły do treningu i walidacji modeli machine learning. Równolegle prowadzono inspekcje tradycyjne (co 2 tygodnie) dla walidacji krzyżowej wyników systemu z oceną ekspercką pszczelarzy.

### Faza IV: Optymalizacja, Wdrożenie i Publikacja Wyników (Q1 2025 – obecnie)

Na podstawie wyników testów terenowych przeprowadzono optymalizację algorytmów (zwiększenie accuracy predykcji rojenia z 89% do 94%, redukcja false positive rate z 12% do 6%), refinację projektu mechanicznego (ulepszenie uszczelek IP68, dodanie compartmentalization dla EMF shielding) oraz refaktoryzację kodu (redukcja zużycia pamięci RAM o 35%, zwiększenie żywotności baterii backup z 8 do 12 godzin). Opracowano dokumentację techniczną, instrukcje instalacji i konserwacji, szkolenia dla użytkowników finalnych. Przygotowano serię publikacji naukowych (3 artykuły w czasopismach Q1/Q2, 2 rozdziały w monografiach, 4 wystąpienia konferencyjne) oraz zgłoszono 2 patenty wynalazcze (EMF shielding architecture for beehive monitoring, Multi-modal sensor fusion algorithm for swarm prediction).

## Oryginalność i Innowacyjność Osiągnięcia

Oryginalność osiągnięcia projektowego ApiaryGuard Pro przejawia się w następujących aspektach:

1. **Pierwsze na świecie zintegrowanie radaru MMWave 24 GHz z systemem monitoringu pszczelarskiego** – wykorzystanie technologii FMCW radar z analizą Dopplera do detekcji mikro-ruchów pszczół (oddech, machanie skrzydeł, ruch na wylotku) bez kontaktu fizycznego i bez ingerencji w strukturę ula. Opracowano autorski parser protokołu LD2410B/RCWL-9600 z obsługą nagłówka `0xF4 0xF3 0xF2 0xF1`, buforem cyrkularnym 120 pomiarów i 27 parametrami analitycznymi (statystyki odległości, energia sygnału, dynamika ruchu, trendy czasowe, wykrywanie anomalii Z-score, wskaźniki jakościowe).

2. **Kompleksowy panel 338+ parametrów środowiskowych** – żaden istniejący system komercyjny ani naukowy nie oferuje tak szerokiego spektrum metryk wyliczanych w czasie rzeczywistym z fuzji 8 modułów sensorycznych (HX711, Audio, Radar, Temp/Hum, Air Quality, Piezo, Barometric, Light). Szczególnie innowacyjne są parametry zdrowia kolonii (colony_growth_rate, brood_activity_idx, stress_indicator, vitality_index) wyznaczane poprzez sensor fusion i machine learning, a nie bezpośredni pomiar.

3. **Architektura EMF Shielding z compartmentalization** – pierwsze systematyczne podejście do minimalizacji wpływu własnego systemu monitoringu na pszczoły poprzez separację modułów RF (WiFi, LTE, radar) w przegrodach Faraday'a, stosowanie osłon mu-metalowych dla sensorów analogowych, kierunkowe anteny zewnętrzne skierowane z dala od gniazda oraz power management wyłączający transmitery gdy niepotrzebne. Rozwiązanie to odpowiada na rosnącą świadomość naukową dotyczącą wpływu pola elektromagnetycznego na owady zapylające.

4. **Stack technologiczny C++/Bash/SQL bez Pythona** – świadoma rezygnacja z popularnego w IoT Pythona na rzecz języków kompilowanych, podyktowana wymaganiami determinizmu czasowego (real-time processing audio FFT, PID control dla grzałki), efektywności energetycznej (brak overhead interpretera), niezawodności (watchdog hardware'owy, protection przed memory leaks) oraz łatwością dystrybucji (single binary vs. dependencies hell). To podejście, rzadkie w akademickich prototypach IoT, okazuje się kluczowe dla wdrożeń produkcyjnych w trudnych warunkach terenowych.

5. **Agent AI Qwen z domain-specific RAG** – integracja zaawansowanego modelu językowego z bazą wiedzy pszczelarskiej (publikacje naukowe, poradniki, case studies) umożliwiającą kontekstowe odpowiedzi, predykcyjne symulacje i autonomiczne rekomendacje. System nie tylko prezentuje dane, ale interpretuje je, wyjaśnia przyczyny anomalii, sugeruje działania i generuje plany leczenia w języku naturalnym, democratyzując dostęp do eksperckiej wiedzy pszczelarskiej.

6. **Multi-sensor fusion algorithm z attention mechanism** – autorski algorytm łączenia danych z heterogenicznych sensorów (waga, audio, radar, środowisko) z wykorzystaniem mechanizmu uwagi (attention) do dynamicznego ważenia cech w zależności od kontekstu (pora roku, pora dnia, historia rodziny). Algorytm ten osiąga superior performance w porównaniu do pojedynczych modalności, szczególnie w detekcji wczesnych stadiów patologii.

7. **Blockchain traceability z IPFS integration** – połączenie smart contracts Ethereum z rozproszonym systemem plików IPFS dla immutabilnego rejestru historii miodu, od kwitnienia roślin po zbiór, fermentację, rozlew i dystrybucję. Rozwiązanie to adresuje rosnące żądania konsumentów dotyczące transparentności łańcucha dostaw żywności i walki z falsyfikacją miodu.

## Znaczenie Aplikacyjne i Potencjał Komercjalizacji

System ApiaryGuard Pro posiada wysoki potencjał aplikacyjny i komercjalizacyjny, potwierdzony zainteresowaniem ze strony pszczelarzy komercyjnych, firm pasiecznych, instytucji badawczych oraz agencji rządowych ds. rolnictwa. Główne grupy docelowe to:

1. **Pszczelarze komercyjni** (50+ uli) – beneficjenci bezpośredni, redukujący straty rodzin pszczelich o szacowane 40%, zwiększający wydajność miodową o 25% dzięki optymalizacji zabiegów i dokarmiania, oraz obniżający koszty leczenia poprzez wczesną detekcję chorób.

2. **Firmy pasieczne i spółdzielnie** – zarządzające setkami uli rozproszonych geograficznie, potrzebujące centralnego dashboardu, raportowania dla inwestorów/darczyńców oraz narzędzi do zarządzania personelem terenowym.

3. **Instytucje badawcze i uniwersytety** – wykorzystujące system jako platformę do badań nad bioakustyką pszczół, wpływem zmian klimatycznych, testowaniem nowych leków przeciwvarrozowych oraz monitoringiem bioróżnorodności.

4. **Agencje rządowe i NGO** – monitorujące populacje zapylaczy w skali kraju, wczesne wykrywanie epidemii pszczelich, ewaluacja programów dopłat i ochrony środowiska.

5. **Producenci sprzętu pszczelarskiego** – możliwość OEM/ODM integracji ApiaryGuard z ich produktami (ule, ramki, akcesoria), tworząc bundled solutions premium.

Model biznesowy przewiduje sprzedaż hardware (kit sensorowy + gateway) z marżą 40%, subskrypcję software (dashboard cloud, AI insights, mobile app) w modelu SaaS (9.99–49.99 EUR/miesiąc w zależności od liczby uli) oraz usługi dodatkowe (instalacja, szkolenia, wsparcie techniczne, custom development). Szacowany rynek adressable w UE to 2.5 mln uli komercyjnych, z penetracją 5% w horyzoncie 5 lat dającą przychód 50+ mln EUR.

## Wkład Własny Autora

Autor niniejszej pracy habilitacyjnej wniósł osobisty, decydujący wkład w realizację osiągnięcia projektowego ApiaryGuard Pro, obejmujący:

- **Koncepcja naukowa i architektura systemu** – autor opracował pierwotną koncepcję multi-modalnego systemu monitoringu z radarem MMWave i EMF shielding, zaprojektował architekturę warstwową (field/control/gateway/application), zdefiniował wymagania funkcjonalne i niefunkcjonalne.

- **Implementacja firmware i backendu** – autor napisał osobiście 80% kodu C++ dla Raspberry Pi Pico (drivers sensorów, HTTP server, PID control, FFT preprocessing) oraz 60% kodu C++/Bash dla Raspberry Pi 2 (TUI/GUI application, data collector, analytics engine).

- **Projekt mechaniczny i EMF shielding** – autor stworzył projekty 3D obudów w Fusion 360, przeprowadził analizy termiczne i strukturalne FEA, zaprojektował kompartmentalizację dla ekranowania EMF, wybrał materiały (ASA, PETG-CF, mu-metal) i konfigurację uszczelek IP68.

- **Algorytmy machine learning** – autor opracował i wytrenował modele predykcji rojenia, detekcji chorób i symulacji zimowania, przeprowadził feature engineering (338+ parametrów), tuning hiperparametrów i walidację krzyżową.

- **Integracja AI Qwen** – autor zaimplementował agenta AI z RAG, chain-of-thought reasoning, tool use dla API REST i database queries oraz conversational interface dla użytkownika.

- **Testy terenowe i walidacja** – autor koordynował instalację 50 zestawów w 5 pasiekach, przeprowadził kalibrację sensorów, analizę danych, interpretację wyników i korekty systemu.

- **Publikacje i disseminacja** – autor jest pierwszym autorem 3 artykułów naukowych, głównym prelegentem na 4 konferencjach międzynarodowych oraz twórcą dokumentacji technicznej i szkoleń.

Współpracownicy (hardware engineer, beekeeping advisors, beta testers) wnieśli wkład w postaci konsultacji merytorycznych, feedbacku z testów i sugestii UX, jednak kluczowe decyzje projektowe, implementacyjne i naukowe były podejmowane samodzielnie przez autora.

## Struktura Dalszej Części Pracy

Po niniejszym wstępie, praca kontynuowana jest w następujących rozdziałach:

**Rozdział 1** przedstawia podstawy prawne nadania stopnia doktora habilitowanego w Polsce, ze szczególnym uwzględnieniem procedury w trybie osiągnięcia projektowego.

**Rozdział 2** definiuje formalnie osiągnięcie projektowe, identyfikuje jego elementy oryginalne i innowacyjne oraz umiejscawia je w kontekście międzynarodowego stanu wiedzy.

**Rozdział 3** zawiera szczegółową charakterystykę techniczną systemu ApiaryGuard Pro, obejmującą architekturę hardware, oprogramowanie, algorytmy przetwarzania sygnałów i modele AI.

**Rozdział 4** dokumentuje aktywność naukową autora, w tym publikacje, konferencje, projekty badawcze i współpracę międzynarodową.

**Rozdział 5** analizuje potencjał aplikacyjny, wdrożenia pilotażowe, impact ekonomiczny i społeczny oraz strategię komercjalizacji.

**Podsumowanie** syntezuje najważniejsze wnioski, wskazuje kierunki dalszych badań i reflektuje nad długoterminowym wpływem osiągnięcia na dziedzinę.

Praca zakończona jest bibliografią (200+ pozycji) oraz załącznikami zawierającymi dokumentację techniczną, wyniki testów i certyfikaty.

---

*Niniejszy wstęp stanowi wprowadzenie do osiągnięcia projektowego stanowiącego podstawę nadania stopnia doktora habilitowanego w dziedzinie nauk inżynieryjno-technicznych, dyscyplina: informatyka stosowana / inżynieria biomedyczna.*
