// Projet Voltaire Bot - OCR (Tesseract) + LLM (Groq / Gemini)
//
// Pipeline :
//   1. Capture d'une zone de l'écran (Win32 GDI) ou lecture du presse-papiers
//   2. Prétraitement de l'image (Leptonica) puis OCR avec Tesseract (modèle français)
//   3. Envoi du texte à deux LLM en parallèle (Groq et Google Gemini)
//   4. Affichage des deux analyses dans la console
//
// Les clés API ne sont jamais écrites dans le code : elles sont lues depuis les
// variables d'environnement ou depuis un fichier config.env placé à côté de l'exécutable.

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>
#include <leptonica/allheaders.h>
#include <nlohmann/json.hpp>
#include <tesseract/baseapi.h>

#include "resource.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "XmlLite.lib")  // requis par libarchive (dépendance de Tesseract)

using json = nlohmann::json;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Console
// ---------------------------------------------------------------------------

enum Color : WORD { DARK_RED = 4, GRAY = 8, GREEN = 10, CYAN = 11, RED = 12, YELLOW = 14, WHITE = 15 };

void SetColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void PrintColored(const std::string& text, WORD color, bool newline = true) {
    SetColor(color);
    std::cout << text;
    if (newline) std::cout << '\n';
    SetColor(WHITE);
}

void Banner() {
    system("cls");
    SetColor(DARK_RED);
    std::cout << "▄▄▄█████▓ ▒█████   ███▄ ▄███▓  ▄████  ██▓     ██▒   █▓\n";
    std::cout << "▓  ██▒ ▓▒▒██▒  ██▒▓██▒▀█▀ ██▒  ██▒ ▀█▒▓██▒    ▓██░   █▒\n";
    std::cout << "▒ ▓██░ ▒░▒██░  ██▒▓██    ▓██░ ▒██░▄▄▄░▒██░     ▓██  █▒░\n";
    std::cout << "░ ▓██▓ ░ ▒██   ██░▒██    ▒██  ░▓█  ██▓▒██░      ▒██ █░░\n";
    std::cout << "  ▒██▒ ░ ░ ████▓▒░▒██▒   ░██▒ ░▒▓███▀▒░██████▒   ▒▀█░\n";
    std::cout << "  ▒ ░░   ░ ▒░▒░▒░ ░ ▒░   ░  ░  ░▒   ▒ ░ ▒░▓  ░   ░ ▐░\n";
    SetColor(GREEN);
    std::cout << "\n  BOT PROJET VOLTAIRE  ·  OCR Tesseract  →  Groq + Gemini\n";
    std::cout << "  ---------------------------------------------------------------\n\n";
    SetColor(WHITE);
}

// ---------------------------------------------------------------------------
// Configuration (variables d'environnement, puis config.env)
// ---------------------------------------------------------------------------

fs::path ExeDirectory() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
}

class Config {
public:
    Config() {
        for (const fs::path& file : { ExeDirectory() / "config.env", fs::current_path() / "config.env" }) {
            std::ifstream in(file);
            std::string line;
            while (std::getline(in, line)) {
                if (line.empty() || line[0] == '#') continue;
                const auto eq = line.find('=');
                if (eq == std::string::npos) continue;
                std::string key = Trim(line.substr(0, eq));
                std::string value = Trim(line.substr(eq + 1));
                if (value.size() >= 2 && (value.front() == '"' || value.front() == '\'') && value.back() == value.front())
                    value = value.substr(1, value.size() - 2);
                fileValues_.emplace(key, value);
            }
        }
    }

    std::string Get(const std::string& key, const std::string& fallback = "") const {
        char* env = nullptr;
        size_t len = 0;
        if (_dupenv_s(&env, &len, key.c_str()) == 0 && env) {
            std::string value(env);
            free(env);
            if (!value.empty()) return value;
        }
        const auto it = fileValues_.find(key);
        return it != fileValues_.end() && !it->second.empty() ? it->second : fallback;
    }

    static std::string Trim(const std::string& s) {
        const auto first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        const auto last = s.find_last_not_of(" \t\r\n");
        return s.substr(first, last - first + 1);
    }

private:
    std::map<std::string, std::string> fileValues_;
};

// ---------------------------------------------------------------------------
// Encodage
// ---------------------------------------------------------------------------

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), out.data(), size, nullptr, nullptr);
    return out;
}

// Nettoie le texte OCR : espaces multiples, lignes vides, caractères de contrôle.
std::string CleanText(const std::string& raw) {
    std::string out;
    bool lastSpace = true;
    for (char c : raw) {
        if (c == '\r') continue;
        const bool space = (c == ' ' || c == '\t' || c == '\n');
        if (space) {
            if (!lastSpace) out += (c == '\n' ? '\n' : ' ');
            lastSpace = true;
        }
        else if ((unsigned char)c >= 0x20) {
            out += c;
            lastSpace = false;
        }
    }
    return Config::Trim(out);
}

// ---------------------------------------------------------------------------
// Presse-papiers (CF_UNICODETEXT pour garder les accents)
// ---------------------------------------------------------------------------

std::string GetClipboardText() {
    if (!OpenClipboard(nullptr)) return "";
    std::string text;
    if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
        if (auto* wide = static_cast<const wchar_t*>(GlobalLock(data))) {
            text = WideToUtf8(wide);
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return text;
}

// ---------------------------------------------------------------------------
// Capture d'écran -> Pix (Leptonica)
// ---------------------------------------------------------------------------

struct PixDeleter { void operator()(Pix* p) const { pixDestroy(&p); } };
using PixPtr = std::unique_ptr<Pix, PixDeleter>;

PixPtr CaptureScreenRegion(RECT r) {
    const int width = r.right - r.left;
    const int height = r.bottom - r.top;
    if (width <= 0 || height <= 0) return nullptr;

    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;  // top-down
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ previous = SelectObject(memory, bitmap);
    BitBlt(memory, 0, 0, width, height, screen, r.left, r.top, SRCCOPY | CAPTUREBLT);

    PixPtr pix(pixCreate(width, height, 32));
    const auto* src = static_cast<const uint8_t*>(bits);
    const int wpl = pixGetWpl(pix.get());
    l_uint32* data = pixGetData(pix.get());
    for (int y = 0; y < height; ++y) {
        l_uint32* line = data + y * wpl;
        const uint8_t* row = src + (size_t)y * width * 4;
        for (int x = 0; x < width; ++x) {
            l_uint32 value;
            composeRGBPixel(row[x * 4 + 2], row[x * 4 + 1], row[x * 4 + 0], &value);  // BGRA -> RGB
            line[x] = value;
        }
    }

    SelectObject(memory, previous);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);
    return pix;
}

// ---------------------------------------------------------------------------
// OCR Tesseract
// ---------------------------------------------------------------------------

class OcrEngine {
public:
    bool Init(const std::string& language, std::string& error) {
        // Ordre de recherche des modèles : à côté de l'exe, ./tessdata, puis TESSDATA_PREFIX.
        std::optional<std::string> dataPath;
        for (const fs::path& dir : { ExeDirectory(), ExeDirectory() / "tessdata", fs::current_path() / "tessdata" }) {
            if (fs::exists(dir / (language.substr(0, language.find('+')) + ".traineddata"))) {
                dataPath = dir.string();
                break;
            }
        }
        if (api_.Init(dataPath ? dataPath->c_str() : nullptr, language.c_str(), tesseract::OEM_LSTM_ONLY) != 0) {
            error = "Impossible de charger le modèle Tesseract '" + language +
                    ".traineddata'. Place-le à côté de l'exécutable ou dans un dossier tessdata/.";
            return false;
        }
        api_.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
        api_.SetVariable("preserve_interword_spaces", "1");
        return true;
    }

    // Prétraitement : niveaux de gris, agrandissement x2, normalisation du contraste.
    // Tesseract fait ensuite sa propre binarisation (Otsu).
    std::string Read(Pix* image) {
        PixPtr gray(pixGetDepth(image) == 32 ? pixConvertRGBToLuminance(image) : pixConvertTo8(image, 0));
        if (!gray) return "";
        PixPtr scaled(pixScaleGrayLI(gray.get(), 2.0f, 2.0f));
        PixPtr normalized(pixContrastNorm(nullptr, scaled ? scaled.get() : gray.get(), 50, 50, 130, 2, 2));
        Pix* input = normalized ? normalized.get() : (scaled ? scaled.get() : gray.get());

        api_.SetImage(input);
        std::unique_ptr<char[]> text(api_.GetUTF8Text());
        const int confidence = api_.MeanTextConf();
        lastConfidence_ = confidence;
        return text ? CleanText(text.get()) : "";
    }

    int LastConfidence() const { return lastConfidence_; }
    ~OcrEngine() { api_.End(); }

private:
    tesseract::TessBaseAPI api_;
    int lastConfidence_ = 0;
};

// ---------------------------------------------------------------------------
// Clients LLM (libcurl + nlohmann/json)
// ---------------------------------------------------------------------------

const char* kSystemPrompt =
    "Tu es un expert de l'orthographe et de la grammaire françaises (niveau Projet Voltaire). "
    "Le texte fourni a été extrait par OCR et peut contenir de petites erreurs de reconnaissance "
    "(lettres confondues, espaces manquants) : ignore-les. "
    "Indique si la phrase contient une faute d'orthographe, de grammaire ou de conjugaison. "
    "Réponds en une ou deux lignes, au format exact : "
    "\"FAUTE : <mot fautif> → <correction> | RÈGLE : <explication courte>\" "
    "ou \"AUCUNE FAUTE | RÈGLE : <pourquoi la phrase est correcte>\".";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

struct HttpResult { long status = 0; std::string body; std::string error; };

HttpResult PostJson(const std::string& url, const json& payload, const std::vector<std::string>& headerLines) {
    HttpResult result;
    CURL* curl = curl_easy_init();
    if (!curl) { result.error = "curl_easy_init a échoué"; return result; }

    const std::string body = payload.dump();
    curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    for (const auto& h : headerLines) headers = curl_slist_append(headers, h.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ProjetVoltaireBot/2.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);

    const CURLcode code = curl_easy_perform(curl);
    if (code != CURLE_OK) result.error = curl_easy_strerror(code);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

// Réessaie automatiquement quand l'API est saturée (429 / 503) ou injoignable.
HttpResult PostJsonWithRetry(const std::string& url, const json& payload, const std::vector<std::string>& headerLines) {
    HttpResult r;
    for (int attempt = 0; attempt < 3; ++attempt) {
        r = PostJson(url, payload, headerLines);
        if (r.error.empty() && r.status != 429 && r.status != 503) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1500 * (attempt + 1)));
    }
    return r;
}

// Extrait un message d'erreur lisible d'une réponse d'API.
std::string ApiError(const HttpResult& r) {
    if (!r.error.empty()) return "Erreur réseau : " + r.error;
    try {
        const auto j = json::parse(r.body);
        if (j.contains("error") && j["error"].contains("message"))
            return "HTTP " + std::to_string(r.status) + " : " + j["error"]["message"].get<std::string>();
    }
    catch (...) {}
    return "HTTP " + std::to_string(r.status);
}

struct LlmAnswer { std::string provider; std::string text; bool ok = false; };

LlmAnswer AskGroq(const std::string& apiKey, const std::string& model, const std::string& text) {
    LlmAnswer answer{ "Groq · " + model };
    if (apiKey.empty()) { answer.text = "GROQ_API_KEY non défini"; return answer; }

    json payload = {
        { "model", model },
        { "temperature", 0.1 },
        { "max_tokens", 1024 },
        { "messages", json::array({
            { { "role", "system" }, { "content", kSystemPrompt } },
            { { "role", "user" }, { "content", text } },
        }) },
    };
    if (model.rfind("openai/gpt-oss", 0) == 0) payload["reasoning_effort"] = "low";  // modèle à raisonnement : réponse rapide
    const HttpResult r = PostJsonWithRetry("https://api.groq.com/openai/v1/chat/completions", payload,
                                  { "Authorization: Bearer " + apiKey });
    if (r.status != 200) { answer.text = ApiError(r); return answer; }
    try {
        answer.text = Config::Trim(json::parse(r.body)["choices"][0]["message"]["content"].get<std::string>());
        answer.ok = true;
    }
    catch (const std::exception& e) { answer.text = std::string("Réponse illisible : ") + e.what(); }
    return answer;
}

LlmAnswer AskGemini(const std::string& apiKey, const std::string& model, const std::string& text) {
    LlmAnswer answer{ "Gemini · " + model };
    if (apiKey.empty()) { answer.text = "GEMINI_API_KEY non défini"; return answer; }

    const json payload = {
        { "systemInstruction", { { "parts", json::array({ { { "text", kSystemPrompt } } }) } } },
        { "contents", json::array({ { { "role", "user" }, { "parts", json::array({ { { "text", text } } }) } } }) },
        { "generationConfig", { { "temperature", 0.2 }, { "maxOutputTokens", 1024 } } },
    };
    const HttpResult r = PostJsonWithRetry(
        "https://generativelanguage.googleapis.com/v1beta/models/" + model + ":generateContent", payload,
        { "x-goog-api-key: " + apiKey });
    if (r.status != 200) { answer.text = ApiError(r); return answer; }
    try {
        const auto j = json::parse(r.body);
        std::string out;
        for (const auto& part : j["candidates"][0]["content"]["parts"])
            if (part.contains("text")) out += part["text"].get<std::string>();
        answer.text = Config::Trim(out);
        answer.ok = !answer.text.empty();
        if (!answer.ok) answer.text = "Réponse vide";
    }
    catch (const std::exception& e) { answer.text = std::string("Réponse illisible : ") + e.what(); }
    return answer;
}

// ---------------------------------------------------------------------------
// Application
// ---------------------------------------------------------------------------

class VoltaireBot {
public:
    explicit VoltaireBot(const Config& config)
        : groqKey_(config.Get("GROQ_API_KEY")),
          geminiKey_(config.Get("GEMINI_API_KEY")),
          groqModel_(config.Get("GROQ_MODEL", "openai/gpt-oss-120b")),
          geminiModel_(config.Get("GEMINI_MODEL", "gemini-flash-lite-latest")),
          ocrLanguage_(config.Get("OCR_LANG", "fra")) {}

    bool InitOcr() {
        std::string error;
        if (!ocr_.Init(ocrLanguage_, error)) { PrintColored("✖ " + error, RED); return false; }
        return true;
    }

    void CheckKeys() const {
        if (groqKey_.empty() && geminiKey_.empty())
            PrintColored("⚠ Aucune clé API trouvée (GROQ_API_KEY / GEMINI_API_KEY). Voir config.env.example.", YELLOW);
    }

    // Analyse un texte avec les deux LLM en parallèle.
    void Analyze(const std::string& text) {
        if (text.size() < 5) { PrintColored("Texte trop court, rien à analyser.", YELLOW); return; }
        PrintColored("\nTexte analysé :", GRAY);
        std::cout << text << "\n\n";

        auto groq = std::async(std::launch::async, AskGroq, groqKey_, groqModel_, text);
        auto gemini = std::async(std::launch::async, AskGemini, geminiKey_, geminiModel_, text);
        Print(groq.get(), CYAN);
        Print(gemini.get(), YELLOW);
        std::cout << "\n";
    }

    void AnalyzeImage(Pix* image) {
        const auto start = std::chrono::steady_clock::now();
        const std::string text = ocr_.Read(image);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        PrintColored("OCR Tesseract : " + std::to_string(ms) + " ms, confiance " + std::to_string(ocr_.LastConfidence()) + " %", GRAY);
        if (text.empty()) { PrintColored("Aucun texte détecté dans la zone.", YELLOW); return; }
        Analyze(text);
    }

    // Mode interactif : F7/F8 définissent la zone, F9 capture, le presse-papiers est surveillé.
    void RunInteractive() {
        Banner();
        CheckKeys();
        std::cout << "Commandes (touches globales) :\n"
                  << "  [F7]  coin HAUT-GAUCHE de la zone (position de la souris)\n"
                  << "  [F8]  coin BAS-DROITE de la zone\n"
                  << "  [F9]  capturer la zone -> OCR -> analyse IA\n"
                  << "  Copier un texte (Ctrl+C) l'analyse directement\n"
                  << "  [Échap] quitter (console au premier plan)\n\n";

        std::string lastClipboard = GetClipboardText();
        POINT topLeft{}, bottomRight{};
        bool hasTopLeft = false, hasBottomRight = false;

        while (true) {
            if (KeyPressed(VK_F7)) {
                GetCursorPos(&topLeft);
                hasTopLeft = true;
                PrintColored("✔ Coin haut-gauche : (" + std::to_string(topLeft.x) + ", " + std::to_string(topLeft.y) + ")", GREEN);
            }
            if (KeyPressed(VK_F8)) {
                GetCursorPos(&bottomRight);
                hasBottomRight = true;
                PrintColored("✔ Coin bas-droite : (" + std::to_string(bottomRight.x) + ", " + std::to_string(bottomRight.y) + ")", GREEN);
            }
            if (KeyPressed(VK_F9)) {
                if (!hasTopLeft || !hasBottomRight) {
                    PrintColored("Définis d'abord la zone avec F7 et F8.", YELLOW);
                }
                else {
                    RECT zone{ std::min(topLeft.x, bottomRight.x), std::min(topLeft.y, bottomRight.y),
                               std::max(topLeft.x, bottomRight.x), std::max(topLeft.y, bottomRight.y) };
                    Banner();
                    PrintColored("Capture de la zone...", GRAY);
                    if (PixPtr pix = CaptureScreenRegion(zone)) AnalyzeImage(pix.get());
                    else PrintColored("Zone invalide.", RED);
                }
            }
            if (KeyPressed(VK_ESCAPE) && GetForegroundWindow() == GetConsoleWindow()) break;

            const std::string clipboard = GetClipboardText();
            if (!clipboard.empty() && clipboard != lastClipboard) {
                lastClipboard = clipboard;
                Banner();
                PrintColored("Nouveau texte dans le presse-papiers", GRAY);
                Analyze(CleanText(clipboard));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(40));
        }
    }

private:
    static void Print(const LlmAnswer& a, WORD color) {
        PrintColored("■ " + a.provider, color);
        PrintColored("  " + a.text, a.ok ? WHITE : RED);
    }

    // Détection de front montant (une seule action par appui).
    bool KeyPressed(int vk) {
        const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        const bool pressed = down && !keyState_[vk];
        keyState_[vk] = down;
        return pressed;
    }

    std::string groqKey_, geminiKey_, groqModel_, geminiModel_, ocrLanguage_;
    OcrEngine ocr_;
    std::map<int, bool> keyState_;
};

void PrintUsage() {
    std::cout << "Usage :\n"
              << "  projet_voltaire.exe                 mode interactif (zone écran + presse-papiers)\n"
              << "  projet_voltaire.exe --image <png>   OCR d'une image puis analyse IA\n"
              << "  projet_voltaire.exe --ocr <png>     OCR seul (affiche le texte reconnu)\n"
              << "  projet_voltaire.exe --text \"...\"    analyse IA d'un texte\n";
}

int wmain(int argc, wchar_t** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);  // coordonnées souris = pixels réels
    curl_global_init(CURL_GLOBAL_DEFAULT);

    const Config config;
    VoltaireBot bot(config);

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.push_back(WideToUtf8(argv[i]));

    int exitCode = 0;
    if (args.empty()) {
        if (bot.InitOcr()) bot.RunInteractive();
        else exitCode = 1;
    }
    else if (args.size() == 2 && (args[0] == "--image" || args[0] == "--ocr")) {
        PixPtr pix(pixRead(args[1].c_str()));
        if (!pix) { PrintColored("Image illisible : " + args[1], RED); exitCode = 1; }
        else if (!bot.InitOcr()) exitCode = 1;
        else if (args[0] == "--image") { bot.CheckKeys(); bot.AnalyzeImage(pix.get()); }
        else {
            OcrEngine ocr;
            std::string error;
            if (!ocr.Init(config.Get("OCR_LANG", "fra"), error)) { PrintColored(error, RED); exitCode = 1; }
            else std::cout << ocr.Read(pix.get()) << "\n(confiance " << ocr.LastConfidence() << " %)\n";
        }
    }
    else if (args.size() == 2 && args[0] == "--text") {
        bot.CheckKeys();
        bot.Analyze(CleanText(args[1]));
    }
    else {
        PrintUsage();
        exitCode = 2;
    }

    curl_global_cleanup();
    return exitCode;
}
