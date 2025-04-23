# 🎧 Аудио Стеганография 📻🔒

> **Простое приложение на C++ с использованием Qt 6** для скрытия текста или файлов в аудиофайлах `.wav` с помощью стеганографии (метод **LSB**).

📌 Разработано в рамках университетского проекта:  
**Артем Деринг** — программирование  
**Алексей Шагабудинов** — отчет

---

## ✨ Возможности

- 📝 Скрытие **текста или любых файлов** (`.png`, `.pdf`, `.zip`, и др.) в WAV-файлах  
- 🔍 Извлечение скрытых данных из WAV  
- 📂 Выбор директории для сохранения  
- 🖼️ Минималистичный и понятный интерфейс  
- 🌐 **Кроссплатформенность:** macOS и Windows  

---

## 🛠️ Требования

### macOS

- 🍺 **Homebrew** — `brew install <пакет>`
- ⚙️ **CMake** ≥ 3.16 — `brew install cmake`
- 🧰 **Qt 6** (6.9.0 или выше) — `brew install qt` или Qt Installer  
- 🔉 **libsndfile** — `brew install libsndfile`
- 💻 **Компилятор C++** — Clang (`xcode-select --install`)

### Windows

- 🧱 **MinGW** (через MSYS2 или Qt Installer)  
- ⚙️ **CMake** ≥ 3.16 — [скачать](https://cmake.org/download/)  
- 🧰 **Qt 6** (6.9.0 или выше)  
- 🔉 **libsndfile** — DLL с [официального сайта](http://www.mega-nerd.com/libsndfile/)

---

## 🚀 Установка и сборка

### 1. Клонируйте репозиторий

```bash
git clone https://github.com/n4p1c/AudioSteganography.git
cd AudioSteganography
