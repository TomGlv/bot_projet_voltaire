#include <iostream>
#include <windows.h>
#include <string>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

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

    std::cout << "Le programme surveille le presse-papiers..." << std::endl;

    std::string lastClipboardText = "";

    while (true) {
        // Récupère le texte actuel du presse-papiers
        std::string currentText = ClipboardSpellChecker::getClipboardText();

        // Vérifie si le contenu du presse-papiers a changé
        if (!currentText.empty() && currentText != lastClipboardText) {
            std::cout << "Nouveau texte détecté dans le presse-papiers : " << currentText << std::endl;

            // Envoyer le texte à Reverso
            std::string response = ClipboardSpellChecker::checkSpelling(currentText);
            if (!response.empty()) {
                std::string correctedText = ClipboardSpellChecker::parseReversoResponse(response);
                std::cout << "Texte corrigé par Reverso : " << correctedText << std::endl;
            }
            else {
                std::cerr << "Erreur : Impossible de récupérer une réponse de Reverso." << std::endl;
            }

            // Met à jour le dernier texte détecté
            lastClipboardText = currentText;
        }

    }

    return 0;
}
