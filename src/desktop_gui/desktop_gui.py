import sys
from urllib.parse import urlparse

from PySide6.QtCore import QSettings, QUrl
from PySide6.QtGui import QAction
from PySide6.QtWidgets import (
    QApplication, QLabel, QLineEdit, QMainWindow, QMessageBox,
    QStatusBar, QToolBar,
)
from PySide6.QtWebEngineCore import QWebEnginePage
from PySide6.QtWebEngineWidgets import QWebEngineView

APP_ORGANIZATION = "ApiaryGuard"
APP_NAME = "HiveMonitorDesktop"
DEFAULT_WEBUI_URL = "http://192.168.1.100/apiary/"


class ApiaryPage(QWebEnginePage):
    """Reject local-file navigation and allow only web origins."""

    def acceptNavigationRequest(self, url, navigation_type, is_main_frame):
        return url.scheme() in ("http", "https", "about")


class HiveMonitorWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.settings = QSettings(APP_ORGANIZATION, APP_NAME)
        self.setWindowTitle("ApiaryGuard — Hive Monitor")
        self.resize(1440, 920)
        self.setMinimumSize(900, 620)

        toolbar = QToolBar("Połączenie")
        toolbar.setMovable(False)
        self.addToolBar(toolbar)
        toolbar.addWidget(QLabel("Adres WebUI: "))

        self.url_input = QLineEdit()
        self.url_input.setMinimumWidth(420)
        self.url_input.setPlaceholderText("https://adres-raspberry/apiary/")
        self.url_input.setClearButtonEnabled(True)
        toolbar.addWidget(self.url_input)

        connect_action = QAction("Połącz", self)
        connect_action.triggered.connect(self.connect_to_webui)
        toolbar.addAction(connect_action)
        reload_action = QAction("Odśwież", self)
        reload_action.triggered.connect(self.reload_page)
        toolbar.addAction(reload_action)

        self.browser = QWebEngineView(self)
        self.browser.setPage(ApiaryPage(self.browser))
        self.browser.loadStarted.connect(lambda: self.set_status("Łączenie z WebUI…"))
        self.browser.loadFinished.connect(self.on_load_finished)
        self.setCentralWidget(self.browser)

        self.setStatusBar(QStatusBar(self))
        self.statusBar().showMessage(
            "Podaj adres WebUI działającego serwera Raspberry Pi i wybierz Połącz."
        )
        self.url_input.setText(str(self.settings.value("webui_url", DEFAULT_WEBUI_URL)))

        about_action = QAction("Informacje", self)
        about_action.triggered.connect(self.show_about)
        toolbar.addAction(about_action)

    def set_status(self, message):
        self.statusBar().showMessage(message)

    def connect_to_webui(self):
        raw_url = self.url_input.text().strip()
        parsed = urlparse(raw_url)
        if parsed.scheme not in ("http", "https") or not parsed.netloc:
            QMessageBox.warning(
                self, "Nieprawidłowy adres",
                "Podaj pełny adres HTTP lub HTTPS do WebUI, np. https://192.168.1.100/apiary/.",
            )
            return
        if parsed.username or parsed.password:
            QMessageBox.warning(
                self, "Nieprawidłowy adres",
                "Dane logowania wpisuj na stronie serwera, a nie w adresie URL.",
            )
            return
        normalized = raw_url if raw_url.endswith("/") else raw_url + "/"
        self.url_input.setText(normalized)
        self.settings.setValue("webui_url", normalized)
        self.browser.setUrl(QUrl(normalized))

    def reload_page(self):
        if self.browser.url().isValid() and not self.browser.url().isEmpty():
            self.browser.reload()
        else:
            self.connect_to_webui()

    def on_load_finished(self, ok):
        if ok:
            self.set_status("WebUI załadowane. Dane i status kolektora pochodzą z serwera.")
        else:
            self.set_status(
                "Nie udało się załadować WebUI. Sprawdź adres, sieć, certyfikat TLS i serwer."
            )

    def show_about(self):
        QMessageBox.information(
            self, "ApiaryGuard Desktop",
            "ApiaryGuard Desktop\nWieloplatformowy klient istniejącego WebUI.\n\n"
            "Aplikacja nie uruchamia drugiego kolektora ani nie symuluje pomiarów. "
            "Łączy się z WebUI/API wdrożonym na serwerze Raspberry Pi lub Linux.",
        )


def main():
    app = QApplication(sys.argv)
    app.setOrganizationName(APP_ORGANIZATION)
    app.setApplicationName(APP_NAME)
    window = HiveMonitorWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
