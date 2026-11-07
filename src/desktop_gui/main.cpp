#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace {
constexpr auto kOrganization = "ApiaryGuard";
constexpr auto kApplication = "HiveMonitorDesktop";
constexpr auto kDefaultWebUi = "http://192.168.1.100/apiary/";
constexpr int kRequestTimeoutMs = 5000;
constexpr int kRefreshIntervalMs = 10000;
}

class SameOriginPage final : public QWebEnginePage {
public:
    explicit SameOriginPage(QObject *parent = nullptr) : QWebEnginePage(parent) {}
    void setOrigin(const QUrl &url) { origin_ = url; }
protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType, bool) override {
        if (url.scheme() == QStringLiteral("about")) return true;
        return (url.scheme() == QStringLiteral("http") ||
                url.scheme() == QStringLiteral("https")) &&
               url.host().compare(origin_.host(), Qt::CaseInsensitive) == 0 &&
               url.port() == origin_.port();
    }
private:
    QUrl origin_;
};

class HiveMonitorWindow final : public QMainWindow {
public:
    HiveMonitorWindow() {
        setWindowTitle(QStringLiteral("ApiaryGuard — Hive Monitor"));
        resize(1440, 920);
        setMinimumSize(960, 640);
        settings_ = new QSettings(kOrganization, kApplication, this);
        network_ = new QNetworkAccessManager(this);
        refreshTimer_ = new QTimer(this);
        refreshTimer_->setInterval(kRefreshIntervalMs);

        auto *toolbar = addToolBar(QStringLiteral("Połączenie"));
        toolbar->setMovable(false);
        toolbar->addWidget(new QLabel(QStringLiteral("Adres WebUI: "), this));
        urlEdit_ = new QLineEdit(this);
        urlEdit_->setMinimumWidth(420);
        urlEdit_->setPlaceholderText(QStringLiteral("https://adres-raspberry/apiary/"));
        toolbar->addWidget(urlEdit_);
        auto *connectAction = toolbar->addAction(QStringLiteral("Połącz"));
        auto *reloadAction = toolbar->addAction(QStringLiteral("Odśwież"));
        toolbar->addSeparator();
        auto *aboutAction = toolbar->addAction(QStringLiteral("Informacje"));

        tabs_ = new QTabWidget(this);
        webView_ = new QWebEngineView(tabs_);
        page_ = new SameOriginPage(webView_);
        webView_->setPage(page_);
        tabs_->addTab(webView_, QStringLiteral("Monitoring"));

        auto *diagnostics = new QWidget(tabs_);
        auto *layout = new QVBoxLayout(diagnostics);
        healthLabel_ = new QLabel(QStringLiteral("Stan kolektora: nie sprawdzono"), diagnostics);
        healthLabel_->setWordWrap(true);
        layout->addWidget(healthLabel_);
        layout->addWidget(new QLabel(
            QStringLiteral("Odpowiedzi API WebUI (health i status):"), diagnostics));
        diagnosticsOutput_ = new QPlainTextEdit(diagnostics);
        diagnosticsOutput_->setReadOnly(true);
        diagnosticsOutput_->setLineWrapMode(QPlainTextEdit::NoWrap);
        layout->addWidget(diagnosticsOutput_);
        auto *checkButton = new QPushButton(
            QStringLiteral("Sprawdź kolektor teraz"), diagnostics);
        layout->addWidget(checkButton);
        tabs_->addTab(diagnostics, QStringLiteral("Diagnostyka"));

        setCentralWidget(tabs_);
        setStatusBar(new QStatusBar(this));
        statusBar()->showMessage(
            QStringLiteral("Podaj adres istniejącego WebUI serwera Raspberry Pi."));
        urlEdit_->setText(settings_->value(
            QStringLiteral("webui_url"), QString::fromLatin1(kDefaultWebUi)).toString());

        connect(connectAction, &QAction::triggered, this, [this] { connectToWebUi(); });
        connect(reloadAction, &QAction::triggered, this, [this] { reload(); });
        connect(aboutAction, &QAction::triggered, this, [this] { showAbout(); });
        connect(checkButton, &QPushButton::clicked, this, [this] { checkCollector(); });
        connect(refreshTimer_, &QTimer::timeout, this, [this] { checkCollector(); });
        connect(webView_, &QWebEngineView::loadStarted, this, [this] {
            statusBar()->showMessage(QStringLiteral("Ładowanie WebUI…"));
        });
        connect(webView_, &QWebEngineView::loadFinished, this, [this](bool ok) {
            statusBar()->showMessage(ok
                ? QStringLiteral("WebUI załadowane.")
                : QStringLiteral("Błąd ładowania WebUI — sprawdź adres, sieć i TLS."));
        });
    }

private:
    bool parseWebUiUrl(QUrl *normalized) {
        const QUrl candidate(urlEdit_->text().trimmed());
        if (!candidate.isValid() ||
            (candidate.scheme() != QStringLiteral("http") &&
             candidate.scheme() != QStringLiteral("https")) ||
            candidate.host().isEmpty() || !candidate.userInfo().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Nieprawidłowy adres"),
                QStringLiteral("Podaj pełny adres HTTP/HTTPS do WebUI. "
                               "Danych logowania nie umieszczaj w adresie URL."));
            return false;
        }
        *normalized = candidate;
        if (!normalized->path().endsWith('/'))
            normalized->setPath(normalized->path() + '/');
        normalized->setFragment(QString());
        return true;
    }

    void connectToWebUi() {
        QUrl url;
        if (!parseWebUiUrl(&url)) return;
        webUiBase_ = url;
        page_->setOrigin(url);
        urlEdit_->setText(url.toString());
        settings_->setValue(QStringLiteral("webui_url"), url.toString());
        webView_->setUrl(url);
        refreshTimer_->start();
        checkCollector();
    }

    void reload() {
        if (webUiBase_.isEmpty()) { connectToWebUi(); return; }
        webView_->reload();
        checkCollector();
    }

    void checkCollector() {
        if (webUiBase_.isEmpty()) return;
        requestApi(QStringLiteral("health"));
        requestApi(QStringLiteral("status"));
    }

    void requestApi(const QString &endpoint) {
        const QUrl url = webUiBase_.resolved(
            QUrl(QStringLiteral("api.php?endpoint=") + endpoint));
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          QStringLiteral("HiveMonitorDesktop/1.0"));
        request.setRawHeader("Accept", "application/json");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::SameOriginRedirectPolicy);
        QNetworkReply *reply = network_->get(request);
        auto *timeout = new QTimer(reply);
        timeout->setSingleShot(true);
        connect(timeout, &QTimer::timeout, reply, [reply] {
            if (!reply->isFinished()) reply->abort();
        });
        connect(reply, &QNetworkReply::finished, this, [this, reply, endpoint, timeout] {
            timeout->stop();
            const QByteArray body = reply->readAll();
            const auto networkError = reply->error();
            const int httpStatus = reply->attribute(
                QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString output;
            if (networkError == QNetworkReply::NoError &&
                httpStatus >= 200 && httpStatus < 300) {
                QJsonParseError parseError;
                const QJsonDocument json = QJsonDocument::fromJson(body, &parseError);
                output = json.isNull() ? QString::fromUtf8(body)
                    : QString::fromUtf8(json.toJson(QJsonDocument::Indented));
                if (endpoint == QStringLiteral("health"))
                    healthLabel_->setText(QStringLiteral("Stan kolektora: odpowiedź HTTP %1")
                                          .arg(httpStatus));
            } else {
                output = QStringLiteral("Błąd: %1 (HTTP %2)\n%3")
                    .arg(reply->errorString()).arg(httpStatus).arg(QString::fromUtf8(body));
                if (endpoint == QStringLiteral("health"))
                    healthLabel_->setText(QStringLiteral("Stan kolektora: brak potwierdzenia (%1)")
                                          .arg(reply->errorString()));
            }
            diagnosticsOutput_->appendPlainText(
                QStringLiteral("[%1] %2\n%3")
                    .arg(endpoint, QDateTime::currentDateTime().toString(Qt::ISODate), output));
            reply->deleteLater();
        });
        timeout->start(kRequestTimeoutMs);
    }

    void showAbout() {
        QMessageBox::information(this, QStringLiteral("ApiaryGuard Desktop"),
            QStringLiteral("Natywna aplikacja C++ / Qt 6.\n\n"
                "Monitoring wyświetla istniejący WebUI. Diagnostyka korzysta "
                "z tych samych endpointów API co WebUI i kolektor używany przez TUI. "
                "Aplikacja nie tworzy drugiej bazy danych ani nie steruje "
                "bezpośrednio wyjściami sprzętowymi."));
    }

    QSettings *settings_ = nullptr;
    QNetworkAccessManager *network_ = nullptr;
    QTimer *refreshTimer_ = nullptr;
    QTabWidget *tabs_ = nullptr;
    QWebEngineView *webView_ = nullptr;
    SameOriginPage *page_ = nullptr;
    QLineEdit *urlEdit_ = nullptr;
    QLabel *healthLabel_ = nullptr;
    QPlainTextEdit *diagnosticsOutput_ = nullptr;
    QUrl webUiBase_;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QString::fromLatin1(kOrganization));
    QApplication::setApplicationName(QString::fromLatin1(kApplication));
    HiveMonitorWindow window;
    window.show();
    return app.exec();
}
