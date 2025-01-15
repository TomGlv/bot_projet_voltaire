#include <iostream>
#include <windows.h>
#include <string>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
#include "resource.h"  // Pour inclure l'identifiant de ton icône
// Définir quelques couleurs pour l'affichage "hacker"
#define GREEN_TEXT 10
#define RED_TEXT 12
#define WHITE_TEXT 15
#define CYAN_TEXT 11
#define YELLOW_TEXT 14
#define BLACK_BACKGROUND 0

// Fonction pour obtenir la largeur de la console
int GetConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;  // Retourne la largeur
}

// Fonction pour afficher du texte centré
void PrintCenteredText(const std::string& text) {
    int consoleWidth = GetConsoleWidth();
    int textLength = text.length();
    int padding = (consoleWidth - textLength) / 2;  // Calcul du padding nécessaire pour centrer le texte

    // Afficher des espaces avant le texte pour le centrer
    for (int i = 0; i < padding; ++i) {
        std::cout << " ";
    }

    // Afficher le texte centré
    std::cout << text << std::endl;
}
// Fonction pour changer la couleur du texte dans la console
void SetConsoleColor(int textColor, int backgroundColor) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (textColor | backgroundColor << 4));
}
// Fonction pour afficher une animation "hacker" de type Matrix sans centrer le texte
void ENterInterface() {
    system("cls");  // Effacer la console
    SetConsoleColor(4, 0);  // Rouge sur fond noir

    std::cout << "▄▄▄█████▓ ▒█████   ███▄ ▄███▓  ▄████  ██▓     ██▒   █▓" << std::endl;
    std::cout << "▓  ██▒ ▓▒▒██▒  ██▒▓██▒▀█▀ ██▒  ██▒ ▀█▒▓██▒    ▓██░   █▒" << std::endl;
    std::cout << "▒ ▓██░ ▒░▒██░  ██▒▓██    ▓██░ ▒██░▄▄▄░▒██░     ▓██  █▒░" << std::endl;
    std::cout << "░ ▓██▓ ░ ▒██   ██░▒██    ▒██  ░▓█  ██▓▒██░      ▒██ █░░" << std::endl;
    std::cout << "  ▒██▒ ░ ░ ████▓▒░▒██▒   ░██▒ ░▒▓███▀▒░██████▒   ▒▀█░" << std::endl;
    std::cout << "  ▒ ░░   ░ ▒░▒░▒░ ░ ▒░   ░  ░  ░▒   ▒ ░ ▒░▓  ░   ░ ▐░" << std::endl;
    std::cout << "    ░      ░ ▒ ▒░ ░  ░      ░   ░   ░ ░ ░ ▒  ░   ░ ░░" << std::endl;
    std::cout << "  ░      ░ ░ ░ ▒  ░      ░   ░  ░   ░   ░ ░        ░░" << std::endl;
    std::cout << "             ░ ░         ░         ░     ░  ░      ░" << std::endl;
    std::cout << " ▄▄▄▄    ▒█████  ▄▄▄█████▓               ██▒   █▓ ▒█████   ██▓    ▄▄▄█████▓ ▄▄▄       ██▓ ██▀███  ▓█████" << std::endl;
    std::cout << "▓█████▄ ▒██▒  ██▒▓  ██▒ ▓▒              ▓██░   █▒▒██▒  ██▒▓██▒    ▓  ██▒ ▓▒▒████▄    ▓██▒▓██ ▒ ██▒▓█   ▀" << std::endl;
    std::cout << "▒██▒ ▄██▒██░  ██▒▒ ▓██░ ▒░               ▓██  █▒░▒██░  ██▒▒██░    ▒ ▓██░ ▒░▒██  ▀█▄  ▒██▒▓██ ░▄█ ▒▒███   " << std::endl;
    std::cout << "▒██░█▀  ▒██   ██░░ ▓██▓ ░                 ▒██ █░░▒██   ██░▒██░    ░ ▓██▓ ░ ░██▄▄▄▄██ ░██░▒██▀▀█▄  ▒▓█  ▄ " << std::endl;
    std::cout << "░▓█  ▀█▓░ ████▓▒░  ▒██▒ ░     ███████      ▒▀█░  ░ ████▓▒░░██████▒  ▒██▒ ░  ▓█   ▓██▒░██░░██▓ ▒██▒░▒████▒" << std::endl;
    std::cout << "░▒▓███▀▒░ ▒░▒░▒░   ▒ ░░                    ░ ▐░  ░ ▒░▒░▒░ ░ ▒░▓  ░  ▒ ░░    ▒▒   ▓▒█░░▓  ░ ▒▓ ░▒▓░░░ ▒░ ░" << std::endl;
    std::cout << "▒░▒   ░   ░ ▒ ▒░     ░                     ░ ░░    ░ ▒ ▒░ ░ ░ ▒  ░    ░      ▒   ▒▒ ░ ▒ ░  ░▒ ░ ▒░ ░ ░  ░" << std::endl;
    std::cout << " ░    ░ ░ ░ ░ ▒    ░                         ░░  ░ ░ ░ ▒    ░ ░     ░        ░   ░▒    ▒ ░  ░░   ░    ░  " << std::endl;
    std::cout << " ░          ░ ░                               ░      ░ ░      ░  ░               ░  ░ ░     ░        ░  ░" << std::endl;
    std::cout << "      ░                                       ░                                                            " << std::endl;
    SetConsoleColor(15, 0);  // Revenir à la couleur blanche sur fond noir
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}

// Fonction pour afficher une animation "hacker" de type Matrix
void hackerInterface() {
    system("cls");  // Effacer la console
    SetConsoleColor(10, 0);  // Vert sur fond noir
    PrintCenteredText("------------------------------------------------------");
    PrintCenteredText("  Fait par Tomglv - BOT PROJET VOLTAIRE");
    PrintCenteredText("------------------------------------------------------");
    SetConsoleColor(15, 0);  // Blanc sur fond noir
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Crée la fenêtre avec l'icône
    HWND hwnd = CreateWindowEx(0, L"NomClasseFenetre", L"TitreFenetre", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr);

    // Charger l'icône à partir de l'ID défini
    HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);  // Définit l'icône de la fenêtre

    // Boucle de messages
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
using json = nlohmann::json;

class ClipboardSpellChecker {
public:
    // Variables statiques pour les parties fixes de la requête
    inline static std::string m_PostHelp = "{\"englishDialect\":\"indifferent\",\"autoReplace\":true,\"getCorrectionDetails\":true,\"interfaceLanguage\":\"fr\",\"locale\":\"\",\"language\":\"fra\",\"text\":\"";
    inline static std::string m_PreHelp = "\",\"originalText\":\"\",\"spellingFeedbackOptions\":{\"insertFeedback\":true,\"userLoggedOn\":false},\"origin\":\"interactive\",\"isHtml\":false,\"IsUserPremium\":false}";

    // Fonction pour lire le texte du presse-papiers
    static std::string getClipboardText() {
        if (!OpenClipboard(nullptr)) return "";
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData == nullptr) {
            CloseClipboard();
            return "";
        }
        char* pszText = static_cast<char*>(GlobalLock(hData));
        if (pszText == nullptr) {
            CloseClipboard();
            return "";
        }
        std::string text(pszText);
        GlobalUnlock(hData);
        CloseClipboard();
        return text;
    }

    // Fonction de rappel pour gérer les données reçues avec libcurl
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
        size_t totalSize = size * nmemb;
        out->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    // Fonction pour envoyer le texte à Reverso et recevoir les résultats
    static std::string checkSpelling(const std::string& text) {
        CURL* curl;
        CURLcode res;
        std::string response;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "https://orthographe.reverso.net/api/v1/Spelling");
            curl_easy_setopt(curl, CURLOPT_POST, 1L);

            // Ajouter le User-Agent personnalisé
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "ClipboardSpellChecker/1.0");

            // Convertir le texte en UTF-8
            std::string postFields = m_PostHelp + utf8Encode(text) + m_PreHelp;

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                return ""; // Si une erreur se produit avec curl, on retourne une chaîne vide.
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        return response;
    }

    // Fonction pour parser la réponse de Reverso
    static std::string parseReversoResponse(const std::string& response) {
        try {
            auto jsonResponse = json::parse(response);

            // Vérification de la présence de "text" dans la réponse JSON
            if (jsonResponse.contains("text")) {
                return jsonResponse["text"].get<std::string>();
            }
            else {
                return "Aucun texte trouvé.";
            }
        }
        catch (const json::exception& e) {
            std::cerr << "Erreur lors du parsing JSON : " << e.what() << std::endl;
            return "Erreur de traitement du texte.";
        }
    }

    // Fonction pour encoder en UTF-8
    static std::string utf8Encode(const std::string& input) {
        std::wstring wideStr(input.begin(), input.end());
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), (int)wideStr.length(), NULL, 0, NULL, NULL);
        std::string utf8Str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), (int)wideStr.length(), &utf8Str[0], size_needed, NULL, NULL);
        return utf8Str;
    }
};

int main() {
    SetConsoleOutputCP(CP_UTF8); // Permet d'afficher correctement les caractères accentués dans la console
    // Afficher l'interface "hacker"
    ENterInterface();
    

    std::cout << "Projet Voltaire Bot :" << std::endl;

    std::string lastClipboardText = "";

    while (true) {
        // Récupère le texte actuel du presse-papiers
        std::string currentText = ClipboardSpellChecker::getClipboardText();

        // Vérifie si le contenu du presse-papiers a changé
        if (!currentText.empty() && currentText != lastClipboardText) {
            hackerInterface();
            std::cout << "\nTexte envoyé : " << currentText << std::endl;

            // Envoyer le texte à Reverso
            std::string response = ClipboardSpellChecker::checkSpelling(currentText);
            if (!response.empty()) {
                std::string correctedText = ClipboardSpellChecker::parseReversoResponse(response);
                std::cout << "\nTexte corrigé : " << correctedText << std::endl;
            }
            else {
                std::cerr << "Erreur : Impossible de récupérer une réponse." << std::endl;
            }

            // Met à jour le dernier texte détecté
            lastClipboardText = currentText;
        }

    }

    return 0;
}
