# Android API — wdrożenie HTTPS

## Wymaganie klienta

Aplikacja Android używa wyłącznie adresów `https://`. Manifest i konfiguracja bezpieczeństwa sieci Android blokują ruch cleartext zarówno w wariancie debug, jak i release. Klient OkHttp zachowuje standardową weryfikację łańcucha certyfikatów i nazwy hosta; nie stosuje zaufania do wszystkich certyfikatów ani własnego `TrustManager`.

Domyślny port klienta to TCP 443. Pole połączenia przyjmuje nazwę DNS albo adres IPv4. Dla nazwy DNS certyfikat serwera musi zawierać zgodny wpis SAN. Przy adresie IPv4 certyfikat musi zawierać ten adres w SAN typu IP.

## Wymagana architektura

Kolektor C++ pozostaje na `127.0.0.1:8080` i nie powinien być wystawiany bezpośrednio do sieci. Należy umieścić przed nim serwer TLS/reverse proxy:
1. Reverse proxy nasłuchuje na TCP 443.
2. Certyfikat ma łańcuch zaufany przez magazyn systemowy Androida i poprawny SAN dla skonfigurowanej nazwy hosta.
3. Proxy przekazuje żądania do `http://127.0.0.1:8080` wyłącznie lokalnie.
4. Zapora dopuszcza z sieci tylko port 443 do proxy; port 8080 pozostaje niedostępny z LAN/WAN.
5. Dostęp do proxy powinien być ograniczony zaporą/VPN lub dodatkowym uwierzytelnieniem odpowiednim dla wdrożenia.

Nie należy włączać `usesCleartextTraffic=true`, dodawać `<trust-anchors src="user">` dla całej aplikacji ani używać klienta akceptującego dowolny certyfikat. Nie należy też umieszczać wspólnego klucza API w kodzie APK — sekret osadzony w aplikacji mobilnej można odzyskać.

## Przykładowa konfiguracja Apache

Na Debianie/Raspberry Pi można użyć Apache jako reverse proxy TLS. Wymagane moduły: `ssl`, `proxy`, `proxy_http`, `headers`. Konfiguracja vhost powinna używać certyfikatu wystawionego dla hosta, który wpisano w aplikacji.

Przykład szkieletu (uzupełnij ścieżki certyfikatu i domenę):

```apache
<VirtualHost *:443>
    ServerName apiary.example.org
    SSLEngine on
    SSLCertificateFile /etc/letsencrypt/live/apiary.example.org/fullchain.pem
    SSLCertificateKeyFile /etc/letsencrypt/live/apiary.example.org/privkey.pem

    SSLProtocol -all +TLSv1.2 +TLSv1.3
    ProxyRequests Off
    ProxyPreserveHost On
    ProxyPass        / http://127.0.0.1:8080/
    ProxyPassReverse / http://127.0.0.1:8080/

    Header always set X-Content-Type-Options "nosniff"
    Header always set Referrer-Policy "no-referrer"
</VirtualHost>
```

Włącz wymagane moduły i zweryfikuj konfigurację przed przeładowaniem Apache. Certyfikat musi być odnawiany automatycznie, a port 80 — jeśli jest używany do ACME — powinien obsługiwać wyłącznie przekierowanie lub walidację certyfikatu.

## Dane lokalne Android

Room przechowuje telemetrię w prywatnym katalogu aplikacji. Kopie zapasowe Androida są wyłączone przez `allowBackup=false`. Nie przechowujemy klucza API w SharedPreferences. W ustawieniach połączenia zapisywany jest jedynie host i port. W razie rozszerzenia aplikacji o dane uwierzytelniające należy użyć Android Keystore i nie umieszczać sekretu w zasobach ani kodzie APK.

## Ważne ograniczenie

Instalator Hive Monitor nie konfiguruje automatycznie publicznego certyfikatu TLS ani reverse proxy, ponieważ wymaga to domeny, DNS i procedury wydania/odnawiania certyfikatu. Po aktualizacji klient Android nie połączy się z samym, nieszyfrowanym portem kolektora 8080 — należy najpierw skonfigurować reverse proxy TLS.
