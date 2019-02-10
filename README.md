# terminal-editor

[![Travis CI Status](https://travis-ci.org/JakubSkowron/terminal-editor.svg?branch=master)](https://travis-ci.org/JakubSkowron/terminal-editor)
[![AppVeyor status](https://ci.appveyor.com/api/projects/status/86ikpll4wy6t5les/branch/master?svg=true)](https://ci.appveyor.com/project/JakubSkowron/terminal-editor/branch/master)

# Requirements

1. Musi działać przez PuTTy.
2. Musi obsługiwać zaznaczanie tekstu:
    - klawiaturą (SHIFT + strzałki/home/end, SHIFT+CTRL + strzałki/home/end, CTRL+A),
    - myszą.
3. Musi obsługiwać CTRL+C, CTRL+V.
4. Jeśli to możliwe schowek powinien być zintegrowany z komputerem który odpala PuTTy.
5. Obsługa tylko LF.
6. Search/replace. Może być bez regeksów.
7. Menu na górze i "okienkowy" interfejs.
8. Konwersja tabs to spaces.
9. Konwersja CRLF to LF.
10. Obługa tabulatorów (niestety konieczność).
11. Jakieś kolorowanie składni (byłoby fajnie, choć nie priorytet).
12. Chodzenie po słowach CTRL + lewo/prawo.
13. Nie musi obsługiwać dużych plików.
14. Musi pokazywać plik jako read only jeśli nie da się zapisać.
15. Smart indent (głupi indent czyli). Tabulator i backspace wstawiają/usuwają 4 spacje. Nowa linia kopiuje indentację z poprzedniej.
16. Podgląd i edycja heksów.
17. Wewnętrzny schowek z historią do kilku wstecz.
18. Wsparcie zmiany rozmiaru okna terminala.
19. cała (prawie) funkcjonalność ma być dostępna bez myszy.
20. Unlimitted UNDO/REDO levels.
21. Support for UTF-8. All other encodings are optional.

# Extra requirements

1. Możliwość dodawania okienek z rozszerzeniami, np. grep, bash console
2. Możliwość zagnieżdżenia w istniejącym programie (tu masz wymiary okienka, daj listnery na zmianę rozmiaru, run)

# Running tests

`editor-tests` executable must be run inside `tests/` directory, because it needs to access `tests/test-data`.

# Used third party libraries

1. Catch2 by Phil Nash (Boost license).
2. nlohmann/json by Niels Lohmann (MIT license).
3. wcwidth by Markus Kuhn (custom license).
4. GSL by Microsoft (MIT license).
5. optional by Simon Brand (CC0 license).
6. expected by Simon Brand (CC0 license).
