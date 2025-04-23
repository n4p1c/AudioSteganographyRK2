#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <QFile>
#include <QFileInfo>

class SteganographyApp : public QMainWindow {
    Q_OBJECT
public:
    SteganographyApp(QWidget *parent = nullptr) : QMainWindow(parent) {
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        QVBoxLayout *layout = new QVBoxLayout(centralWidget);

        textInput = new QLineEdit(this);
        textInput->setPlaceholderText("Введите текст для скрытия (или оставьте пустым для файла)");
        layout->addWidget(textInput);

        QPushButton *selectAudioButton = new QPushButton("Выбрать аудиофайл", this);
        connect(selectAudioButton, &QPushButton::clicked, this, &SteganographyApp::selectAudioFile);
        layout->addWidget(selectAudioButton);

        QPushButton *selectFileButton = new QPushButton("Выбрать файл для скрытия", this);
        connect(selectFileButton, &QPushButton::clicked, this, &SteganographyApp::selectInputFile);
        layout->addWidget(selectFileButton);

        QPushButton *encryptButton = new QPushButton("Скрыть данные", this);
        connect(encryptButton, &QPushButton::clicked, this, &SteganographyApp::encryptData);
        layout->addWidget(encryptButton);

        QPushButton *decryptButton = new QPushButton("Извлечь данные", this);
        connect(decryptButton, &QPushButton::clicked, this, &SteganographyApp::decryptData);
        layout->addWidget(decryptButton);

        setWindowTitle("Аудио Стеганография");
        resize(400, 250);
    }

private slots:
    void selectAudioFile() {
        audioFilePath = QFileDialog::getOpenFileName(this, "Выбрать аудиофайл", "", "Аудиофайлы (*.wav)");
        if (!audioFilePath.isEmpty()) {
            QMessageBox::information(this, "Успех", "Аудиофайл выбран: " + audioFilePath);
        }
    }

    void selectInputFile() {
        inputFilePath = QFileDialog::getOpenFileName(this, "Выбрать файл для скрытия", "", "Все файлы (*)");
        if (!inputFilePath.isEmpty()) {
            QMessageBox::information(this, "Успех", "Файл выбран: " + inputFilePath);
        }
    }

    void encryptData() {
        if (audioFilePath.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Сначала выберите аудиофайл!");
            return;
        }

        QByteArray dataToHide;
        bool isText = true;
        QString fileName;

        QString text = textInput->text();
        if (!text.isEmpty()) {
            dataToHide = text.toUtf8();
            fileName = "text.txt";
        } else if (!inputFilePath.isEmpty()) {
            QFile file(inputFilePath);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл для скрытия!");
                return;
            }
            dataToHide = file.readAll();
            file.close();
            isText = false;
            QFileInfo fileInfo(inputFilePath);
            fileName = fileInfo.fileName();
        } else {
            QMessageBox::warning(this, "Ошибка", "Введите текст или выберите файл для скрытия!");
            return;
        }

        SF_INFO sfinfo;
        sfinfo.format = 0;
        SNDFILE *infile = sf_open(audioFilePath.toStdString().c_str(), SFM_READ, &sfinfo);
        if (!infile) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть аудиофайл!");
            return;
        }

        std::vector<short> audioData(sfinfo.frames * sfinfo.channels);
        sf_read_short(infile, audioData.data(), sfinfo.frames * sfinfo.channels);
        sf_close(infile);

        QByteArray fileNameBytes = fileName.toUtf8().left(32);
        while (fileNameBytes.size() < 32) {
            fileNameBytes.append('\0');
        }

        size_t maxBits = audioData.size();
        size_t requiredBits = (dataToHide.size() + fileNameBytes.size() + 8) * 8 + 64;
        if (requiredBits > maxBits) {
            QMessageBox::warning(this, "Ошибка", "Аудиофайл слишком мал для данных!");
            return;
        }

        std::vector<char> header;
        uint32_t dataLength = dataToHide.size();
        header.push_back((dataLength >> 24) & 0xFF);
        header.push_back((dataLength >> 16) & 0xFF);
        header.push_back((dataLength >> 8) & 0xFF);
        header.push_back(dataLength & 0xFF);
        header.push_back(isText ? 0 : 1);
        for (char c : fileNameBytes) {
            header.push_back(c);
        }

        size_t bitIndex = 0;
        for (char byte : header) {
            for (int i = 7; i >= 0 && bitIndex < audioData.size(); --i) {
                char bit = (byte >> i) & 1;
                audioData[bitIndex] = (audioData[bitIndex] & ~1) | bit;
                bitIndex++;
            }
        }

        for (char byte : dataToHide) {
            for (int i = 7; i >= 0 && bitIndex < audioData.size(); --i) {
                char bit = (byte >> i) & 1;
                audioData[bitIndex] = (audioData[bitIndex] & ~1) | bit;
                bitIndex++;
            }
        }

        QString outputPath = QFileDialog::getSaveFileName(this, "Сохранить аудиофайл", "", "Аудиофайлы (*.wav)");
        if (outputPath.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Не выбран путь для сохранения!");
            return;
        }

        SNDFILE *outfile = sf_open(outputPath.toStdString().c_str(), SFM_WRITE, &sfinfo);
        if (!outfile) {
            QMessageBox::warning(this, "Ошибка", "Не удалось сохранить аудиофайл!");
            return;
        }
        sf_write_short(outfile, audioData.data(), audioData.size());
        sf_close(outfile);

        QMessageBox::information(this, "Успех", "Данные скрыты в " + outputPath);
    }

    void decryptData() {
        if (audioFilePath.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Сначала выберите аудиофайл!");
            return;
        }

        SF_INFO sfinfo;
        sfinfo.format = 0;
        SNDFILE *infile = sf_open(audioFilePath.toStdString().c_str(), SFM_READ, &sfinfo);
        if (!infile) {
            QMessageBox::warning(this, "Ошибка", "Не удалось открыть аудиофайл!");
            return;
        }

        std::vector<short> audioData(sfinfo.frames * sfinfo.channels);
        sf_read_short(infile, audioData.data(), sfinfo.frames * sfinfo.channels);
        sf_close(infile);

        uint32_t dataLength = 0;
        char dataType = 0;
        QByteArray fileNameBytes;
        size_t bitIndex = 0;

        for (int i = 0; i < 4; ++i) {
            char byte = 0;
            for (int j = 0; j < 8 && bitIndex < audioData.size(); ++j) {
                byte = (byte << 1) | (audioData[bitIndex] & 1);
                bitIndex++;
            }
            dataLength = (dataLength << 8) | (unsigned char)byte;
        }

        for (int j = 0; j < 8 && bitIndex < audioData.size(); ++j) {
            dataType = (dataType << 1) | (audioData[bitIndex] & 1);
            bitIndex++;
        }

        for (int i = 0; i < 32; ++i) {
            char byte = 0;
            for (int j = 0; j < 8 && bitIndex < audioData.size(); ++j) {
                byte = (byte << 1) | (audioData[bitIndex] & 1);
                bitIndex++;
            }
            fileNameBytes.append(byte);
        }

        QString fileName = QString::fromUtf8(fileNameBytes).trimmed();

        if (dataLength * 8 + bitIndex > audioData.size()) {
            QMessageBox::warning(this, "Ошибка", "Некорректные данные в аудиофайле!");
            return;
        }

        QByteArray extractedData;
        for (size_t i = 0; i < dataLength && bitIndex < audioData.size(); ++i) {
            char byte = 0;
            for (int j = 0; j < 8 && bitIndex < audioData.size(); ++j) {
                byte = (byte << 1) | (audioData[bitIndex] & 1);
                bitIndex++;
            }
            extractedData.append(byte);
        }

        if (dataType == 0) {
            QString text = QString::fromUtf8(extractedData);
            QMessageBox::information(this, "Извлеченный текст", "Текст: " + text);
        } else {
            QString outputPath = QFileDialog::getSaveFileName(this, "Сохранить извлеченный файл", fileName, "Все файлы (*)");
            if (outputPath.isEmpty()) {
                QMessageBox::warning(this, "Ошибка", "Не выбран путь для сохранения!");
                return;
            }

            QFile file(outputPath);
            if (!file.open(QIODevice::WriteOnly)) {
                QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл!");
                return;
            }
            file.write(extractedData);
            file.close();

            QMessageBox::information(this, "Успех", "Файл сохранен: " + outputPath);
        }
    }

private:
    QLineEdit *textInput;
    QString audioFilePath;
    QString inputFilePath;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SteganographyApp window;
    window.show();
    return app.exec();
}

#include "main.moc"